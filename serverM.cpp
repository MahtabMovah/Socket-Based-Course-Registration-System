/**
 * @file serverMain.cpp
 * @brief Central / main server.
 *
 * Responsibilities:
 *  1. Accept a TCP connection from the client.
 *  2. Collect and encrypt credentials, then authenticate via ServerC (UDP).
 *  3. Allow up to MAX_AUTH_ATTEMPTS login tries; send the result back to the client.
 *  4. After successful login, forward course queries to the appropriate
 *     department server (ServerEE or ServerCS) over UDP and relay the
 *     response back to the client over TCP.
 *
 * Port map:
 *   TCP  25289  — client connections
 *   UDP  24289  — this server's UDP socket (main)
 *   UDP  21289  — ServerC  (authentication)
 *   UDP  22289  — ServerCS (CS  department queries)
 *   UDP  23289  — ServerEE (EE  department queries)
 *
 * Build:  g++ -std=c++17 -Wall -Wextra -o serverMain serverMain.cpp
 * Usage:  ./serverMain
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

// ─── Configuration ────────────────────────────────────────────────────────────

static constexpr const char* LOCAL_IP          = "127.0.0.1";
static constexpr uint16_t    TCP_CLIENT_PORT   = 25289;
static constexpr uint16_t    UDP_MAIN_PORT     = 24289;
static constexpr uint16_t    UDP_SERVERC_PORT  = 21289;
static constexpr uint16_t    UDP_SERVERCS_PORT = 22289;
static constexpr uint16_t    UDP_SERVEREE_PORT = 23289;
static constexpr int         MAX_AUTH_ATTEMPTS = 3;
static constexpr size_t      SMALL_BUF         = 50;
static constexpr size_t      LARGE_BUF         = 30000;

// ─── Auth result tokens ───────────────────────────────────────────────────────

static constexpr const char* AUTH_OK        = "TT";
static constexpr const char* AUTH_PASS_ONLY = "T";   // pass ok, wrong user
static constexpr const char* AUTH_FAIL      = "F";   // no match

// Token relayed back to client
static constexpr const char* CLIENT_AUTH_OK        = "TT";
static constexpr const char* CLIENT_AUTH_FAIL_PASS = "FP";  // password wrong
static constexpr const char* CLIENT_AUTH_FAIL_USER = "FU";  // user not found

// ─── Encryption ───────────────────────────────────────────────────────────────

/**
 * @brief Apply the Caesar-style credential encryption used by this system.
 *
 * Shift rules (matching ServerC's expected encoding):
 *   a–w  (+4),  x–z  (-22)
 *   A–V  (+4),  W–Z  (-22)
 *   0–5  (+4),  6–9  (-6)
 *   everything else unchanged
 *
 * @param input  Plaintext string.
 * @return Encrypted string of the same length.
 */
static std::string encryptCredential(const std::string& input) {
    std::string output(input.size(), '\0');

    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);

        if      (c >= 'a' && c <= 'w') output[i] = static_cast<char>(c + 4);
        else if (c >= 'x' && c <= 'z') output[i] = static_cast<char>(c - 22);
        else if (c >= 'A' && c <= 'V') output[i] = static_cast<char>(c + 4);
        else if (c >= 'W' && c <= 'Z') output[i] = static_cast<char>(c - 22);
        else if (c >= '0' && c <= '5') output[i] = static_cast<char>(c + 4);
        else if (c >= '6' && c <= '9') output[i] = static_cast<char>(c - 6);
        else                           output[i] = static_cast<char>(c);
    }

    return output;
}

// ─── Address helpers ──────────────────────────────────────────────────────────

/** @brief Fill a sockaddr_in for a given port (all on LOCAL_IP). */
static sockaddr_in makeAddr(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = ::inet_addr(LOCAL_IP);
    return addr;
}

// ─── I/O helpers ──────────────────────────────────────────────────────────────

/**
 * @brief Send a string over a TCP socket.
 * @return true on success.
 */
static bool tcpSend(int fd, const std::string& msg) {
    if (::send(fd, msg.c_str(), msg.size(), 0) == -1) {
        ::perror("send (TCP)");
        return false;
    }
    return true;
}

/**
 * @brief Receive a null-terminated string from a TCP socket.
 * @return Number of bytes received, or -1 on error.
 */
static ssize_t tcpRecv(int fd, char* buf, size_t bufLen) {
    ssize_t n = ::recv(fd, buf, bufLen - 1, 0);
    if (n == -1) { ::perror("recv (TCP)"); return -1; }
    buf[n] = '\0';
    return n;
}

/**
 * @brief Send a string via UDP to a pre-configured destination.
 * @return true on success.
 */
static bool udpSend(int fd, const std::string& msg, const sockaddr_in& dest) {
    if (::sendto(fd, msg.data(), msg.size(), 0,
                 reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) == -1) {
        ::perror("sendto (UDP)");
        return false;
    }
    return true;
}

/**
 * @brief Receive a null-terminated string via UDP.
 * @return Number of bytes received, or -1 on error.
 */
static ssize_t udpRecv(int fd, char* buf, size_t bufLen) {
    sockaddr_in sender{};
    socklen_t   senderLen = sizeof(sender);
    ssize_t n = ::recvfrom(fd, buf, bufLen - 1, 0,
                           reinterpret_cast<sockaddr*>(&sender), &senderLen);
    if (n == -1) { ::perror("recvfrom (UDP)"); return -1; }
    buf[n] = '\0';
    return n;
}

// ─── Socket initialisation ────────────────────────────────────────────────────

/**
 * @brief Create, bind, and listen on the TCP socket for client connections.
 * @return Listening socket fd, or -1 on failure.
 */
static int createTcpListener() {
    int fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) { ::perror("socket (TCP)"); return -1; }

    // Allow fast restart without "Address already in use"
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = makeAddr(TCP_CLIENT_PORT);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::perror("bind (TCP)"); ::close(fd); return -1;
    }
    if (::listen(fd, 5) == -1) {
        ::perror("listen (TCP)"); ::close(fd); return -1;
    }
    return fd;
}

/**
 * @brief Create and bind the UDP socket used for all backend communication.
 * @return Bound socket fd, or -1 on failure.
 */
static int createUdpSocket() {
    int fd = ::socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1) { ::perror("socket (UDP)"); return -1; }

    sockaddr_in addr = makeAddr(UDP_MAIN_PORT);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::perror("bind (UDP)"); ::close(fd); return -1;
    }
    return fd;
}

// ─── Authentication phase ─────────────────────────────────────────────────────

/**
 * @brief Run the authentication handshake for one connection attempt.
 *
 * Prompts the client for credentials over TCP, encrypts them, sends to
 * ServerC over UDP, and returns the result token from ServerC.
 *
 * @param clientFd  Accepted TCP client socket.
 * @param udpFd     Bound UDP socket.
 * @param addrC     Pre-configured address of ServerC.
 * @param[out] usernameOut  Plain username entered by the client.
 * @return ServerC result token ("TT", "T", or "F"), or empty on I/O error.
 */
static std::string runAuthAttempt(int clientFd, int udpFd,
                                  const sockaddr_in& addrC,
                                  std::string& usernameOut) {
    char buf[SMALL_BUF];

    // ── Ask for / receive username ────────────────────────────────────────────
    if (!tcpSend(clientFd, "Please enter the username: ")) return {};
    if (tcpRecv(clientFd, buf, SMALL_BUF) == -1) return {};
    usernameOut = buf;

    std::cout << "The main server received the authentication for "
              << usernameOut << " using TCP over port " << TCP_CLIENT_PORT << ".\n";

    // ── Ask for / receive password ────────────────────────────────────────────
    if (!tcpSend(clientFd, "Please enter the password: ")) return {};
    if (tcpRecv(clientFd, buf, SMALL_BUF) == -1) return {};
    std::string password(buf);

    // ── Encrypt and forward to ServerC ───────────────────────────────────────
    std::string encUser = encryptCredential(usernameOut);
    std::string encPass = encryptCredential(password);

    if (!udpSend(udpFd, encUser, addrC)) return {};
    if (!udpSend(udpFd, encPass, addrC)) return {};

    std::cout << "The main server sent an authentication request to ServerC.\n";

    // ── Receive result from ServerC ───────────────────────────────────────────
    char authBuf[SMALL_BUF];
    if (udpRecv(udpFd, authBuf, SMALL_BUF) == -1) return {};

    std::cout << "The main server received the result of the authentication "
                 "request from ServerC using UDP over port "
              << UDP_SERVERC_PORT << ".\n";

    return std::string(authBuf);
}

// ─── Query phase ──────────────────────────────────────────────────────────────

/**
 * @brief Determine the department prefix of a course code.
 * @return "EE", "CS", or "" if unrecognised (defaults to EE server).
 */
static std::string departmentOf(const std::string& courseCode) {
    if (courseCode.size() >= 2) {
        std::string prefix = courseCode.substr(0, 2);
        if (prefix == "EE") return "EE";
        if (prefix == "CS") return "CS";
    }
    return "";
}

/**
 * @brief Run the query loop after successful authentication.
 *
 * Receives (course_code, category) pairs from the client over TCP,
 * routes them to the correct department server over UDP, and relays
 * the response back to the client.
 *
 * @param clientFd  Authenticated TCP client socket.
 * @param udpFd     Bound UDP socket.
 * @param addrCS    Pre-configured address of ServerCS.
 * @param addrEE    Pre-configured address of ServerEE.
 * @param username  Authenticated username (for log output).
 */
static void runQueryLoop(int clientFd, int udpFd,
                         const sockaddr_in& addrCS,
                         const sockaddr_in& addrEE,
                         const std::string& username) {
    char codeBuf[SMALL_BUF];
    char catBuf[SMALL_BUF];
    char resultBuf[LARGE_BUF];

    while (true) {
        // ── Receive course code ───────────────────────────────────────────────
        if (tcpRecv(clientFd, codeBuf, SMALL_BUF) <= 0) break;
        std::string courseCode(codeBuf);

        // ── Receive category ──────────────────────────────────────────────────
        if (tcpRecv(clientFd, catBuf, SMALL_BUF) <= 0) break;
        std::string category(catBuf);

        std::cout << "The main server received from " << username
                  << " to query course " << courseCode
                  << " about " << category
                  << " using TCP over port " << TCP_CLIENT_PORT << ".\n";

        // ── Route to department server ────────────────────────────────────────
        std::string dept = departmentOf(courseCode);
        const sockaddr_in& dest = (dept == "CS") ? addrCS : addrEE;
        const std::string  deptName = (dept == "CS") ? "ServerCS" : "ServerEE";
        uint16_t           deptPort = (dept == "CS") ? UDP_SERVERCS_PORT : UDP_SERVEREE_PORT;

        std::cout << "The main server sent a request to " << deptName << ".\n";

        if (!udpSend(udpFd, courseCode, dest)) break;
        if (!udpSend(udpFd, category,   dest)) break;

        // ── Receive result from department server ─────────────────────────────
        if (udpRecv(udpFd, resultBuf, LARGE_BUF) == -1) break;

        std::cout << "The main server received the response from "
                  << deptName << " using UDP over port " << deptPort << ".\n";

        // ── Relay result to client ────────────────────────────────────────────
        std::cout << "The main server sent the query information to the client.\n\n";
        if (!tcpSend(clientFd, std::string(resultBuf))) break;
    }
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main() {
    int tcpFd = createTcpListener();
    if (tcpFd == -1) return 1;

    int udpFd = createUdpSocket();
    if (udpFd == -1) { ::close(tcpFd); return 1; }

    std::cout << "The main server is up and running.\n";

    // Pre-configure backend addresses
    sockaddr_in addrC  = makeAddr(UDP_SERVERC_PORT);
    sockaddr_in addrCS = makeAddr(UDP_SERVERCS_PORT);
    sockaddr_in addrEE = makeAddr(UDP_SERVEREE_PORT);

    // ── Accept one client connection ──────────────────────────────────────────
    sockaddr_in clientAddr{};
    socklen_t   clientLen = sizeof(clientAddr);
    int clientFd = ::accept(tcpFd,
                            reinterpret_cast<sockaddr*>(&clientAddr),
                            &clientLen);
    if (clientFd == -1) {
        ::perror("accept");
        ::close(tcpFd);
        ::close(udpFd);
        return 1;
    }

    // ── Authentication loop ───────────────────────────────────────────────────
    bool authenticated = false;
    int  attemptsLeft  = MAX_AUTH_ATTEMPTS;

    while (attemptsLeft > 0 && !authenticated) {
        std::string username;
        std::string token = runAuthAttempt(clientFd, udpFd, addrC, username);

        if (token.empty()) break;   // I/O error

        if (token == AUTH_OK) {
            authenticated = true;
            if (!tcpSend(clientFd, CLIENT_AUTH_OK)) break;
            std::cout << "The main server sent the authentication result "
                         "to the client.\n\n";
            runQueryLoop(clientFd, udpFd, addrCS, addrEE, username);

        } else {
            --attemptsLeft;
            const char* clientToken =
                (token == AUTH_PASS_ONLY) ? CLIENT_AUTH_FAIL_PASS
                                          : CLIENT_AUTH_FAIL_USER;
            tcpSend(clientFd, clientToken);
            std::cout << "The main server sent the authentication result "
                         "to the client.\n";
            if (attemptsLeft > 0) {
                std::cout << "Attempts remaining: " << attemptsLeft << ".\n\n";
            }
        }
    }

    ::close(clientFd);
    ::close(tcpFd);
    ::close(udpFd);
    return authenticated ? 0 : 1;
}
