/**
 * @file client.cpp
 * @brief TCP client for course query system with authentication.
 *
 * Connects to a central server, authenticates the user (up to 3 attempts),
 * then allows repeated course-category queries until the session ends.
 *
 * Build:  g++ -std=c++17 -Wall -Wextra -o client client.cpp
 * Usage:  ./client
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

// ─── Configuration ────────────────────────────────────────────────────────────

static constexpr uint16_t    SERVER_PORT      = 25289;
static constexpr const char* SERVER_IP        = "127.0.0.1";
static constexpr size_t      BUF_LARGE        = 30000;
static constexpr size_t      BUF_AUTH         = 4;
static constexpr int         MAX_AUTH_ATTEMPTS = 3;

// ─── Auth result tokens (must match server) ───────────────────────────────────

static constexpr const char* AUTH_OK        = "TT";
static constexpr const char* AUTH_FAIL_USER = "FU";   // username not found
static constexpr const char* AUTH_FAIL_PASS = "FP";   // password mismatch

// ─── Helpers ──────────────────────────────────────────────────────────────────

/**
 * @brief Send an entire string over a connected socket.
 * @return true on success, false on error.
 */
static bool sendStr(int fd, const std::string& msg) {
    if (::send(fd, msg.c_str(), msg.size(), 0) == -1) {
        ::perror("send");
        return false;
    }
    return true;
}

/**
 * @brief Receive a null-terminated string from a connected socket.
 * @param fd     Socket file descriptor.
 * @param buf    Destination buffer.
 * @param bufLen Buffer capacity (including null terminator).
 * @return Number of bytes received, or -1 on error.
 */
static ssize_t recvStr(int fd, char* buf, size_t bufLen) {
    ssize_t n = ::recv(fd, buf, bufLen - 1, 0);
    if (n == -1) {
        ::perror("recv");
    } else {
        buf[n] = '\0';
    }
    return n;
}

/**
 * @brief Read a single line from stdin into a std::string.
 */
static std::string readLine() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// ─── Socket initialisation ────────────────────────────────────────────────────

/**
 * @brief Create and connect a TCP socket to the server.
 * @return Connected socket file descriptor, or -1 on failure.
 */
static int createAndConnect() {
    int fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        ::perror("socket");
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(SERVER_PORT);
    addr.sin_addr.s_addr = ::inet_addr(SERVER_IP);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::perror("connect");
        ::close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Retrieve the local port number assigned to a socket.
 * @return Port number in host byte order, or 0 on error.
 */
static uint16_t localPort(int fd) {
    sockaddr_in sin{};
    socklen_t   len = sizeof(sin);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&sin), &len) == -1) {
        ::perror("getsockname");
        return 0;
    }
    return ntohs(sin.sin_port);
}

// ─── Session logic ────────────────────────────────────────────────────────────

/**
 * @brief Run the interactive course-query loop after successful authentication.
 *
 * @param fd       Authenticated socket.
 * @param username Authenticated username (for display).
 * @param port     Local TCP port (for display).
 */
static void runQuerySession(int fd, const std::string& username, uint16_t port) {
    char responseBuf[BUF_LARGE];

    while (true) {
        // ── Course code ──────────────────────────────────────────────────────
        std::cout << "Please enter the course code to query: ";
        std::string courseCode = readLine();
        if (courseCode.empty()) continue;

        if (!sendStr(fd, courseCode)) break;

        // ── Category ─────────────────────────────────────────────────────────
        std::cout << "Please enter the category "
                     "(Credit / Professor / Days / CourseName): ";
        std::string category = readLine();
        if (category.empty()) continue;

        if (!sendStr(fd, category)) break;

        std::cout << username << " sent a request to the main server.\n";

        // ── Response ─────────────────────────────────────────────────────────
        if (recvStr(fd, responseBuf, BUF_LARGE) == -1) break;

        std::cout << "The client received the response from the main server "
                     "using TCP over port " << port << ".\n";
        std::cout << category << " of " << courseCode
                  << " is " << responseBuf << "\n\n";
    }
}

/**
 * @brief Perform one authentication attempt.
 *
 * Exchanges username and password with the server, then returns the
 * server's response token.
 *
 * @param fd         Connected socket.
 * @param[out] usernameOut  Username entered by the user (for later display).
 * @return Server auth token string (e.g. "TT", "FU", "FP"),
 *         or an empty string on I/O error.
 */
static std::string authenticate(int fd, std::string& usernameOut) {
    char promptBuf[BUF_LARGE];
    char authBuf[BUF_AUTH];

    // ── Username prompt ───────────────────────────────────────────────────────
    if (recvStr(fd, promptBuf, BUF_LARGE) == -1) return {};
    std::cout << promptBuf;

    usernameOut = readLine();
    if (!sendStr(fd, usernameOut)) return {};

    // ── Password prompt ───────────────────────────────────────────────────────
    if (recvStr(fd, promptBuf, BUF_LARGE) == -1) return {};
    std::cout << promptBuf;

    std::string password = readLine();
    if (!sendStr(fd, password)) return {};

    std::cout << usernameOut << " sent an authentication request to the main server.\n";

    // ── Auth result ───────────────────────────────────────────────────────────
    if (recvStr(fd, authBuf, BUF_AUTH) == -1) return {};
    return std::string(authBuf);
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main() {
    std::cout << "The client is up and running.\n";

    int  attemptsLeft = MAX_AUTH_ATTEMPTS;
    bool authenticated = false;

    while (attemptsLeft > 0 && !authenticated) {
        int fd = createAndConnect();
        if (fd == -1) return 1;

        uint16_t    port = localPort(fd);
        std::string username;
        std::string authToken = authenticate(fd, username);

        if (authToken.empty()) {
            // I/O error — abort entirely
            ::close(fd);
            return 1;
        }

        std::cout << username << " received the result of authentication "
                     "using TCP over port " << port << ".\n";

        if (authToken == AUTH_OK) {
            std::cout << "Authentication is successful.\n\n";
            authenticated = true;
            runQuerySession(fd, username, port);
        } else {
            --attemptsLeft;
            std::cout << "Authentication failed.\n";

            if (authToken == AUTH_FAIL_USER) {
                std::cout << "Username does not exist.\n";
            } else if (authToken == AUTH_FAIL_PASS) {
                std::cout << "Password does not match.\n";
            }

            if (attemptsLeft > 0) {
                std::cout << "Attempts remaining: " << attemptsLeft << "\n\n";
            } else {
                std::cout << "Authentication failed for "
                          << MAX_AUTH_ATTEMPTS
                          << " attempts. Client will shut down.\n";
            }
        }

        ::close(fd);
    }

    return authenticated ? 0 : 1;
}
