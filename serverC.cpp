/**
 * @file serverC.cpp
 * @brief Authentication server (ServerC).
 *
 * Listens on a UDP socket for (username, password) pairs forwarded by the
 * main server, validates them against a comma-separated credential file
 * ("cred.txt"), and replies with a result token:
 *
 *   "TT" — username and password both match
 *   "T"  — password matches but username does not
 *   "F"  — neither matches
 *
 * Build:  g++ -std=c++17 -Wall -Wextra -o serverC serverC.cpp
 * Usage:  ./serverC
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// ─── Configuration ────────────────────────────────────────────────────────────

static constexpr const char* SERVER_IP       = "127.0.0.1";
static constexpr uint16_t    AUTH_UDP_PORT   = 21289;   // this server's port
static constexpr uint16_t    MAIN_UDP_PORT   = 24289;   // main server's port
static constexpr size_t      RECV_BUF_SIZE   = 51;
static constexpr const char* CREDENTIAL_FILE = "cred.txt";

// ─── Auth result tokens (must match main server & client) ─────────────────────

static constexpr const char* AUTH_OK        = "TT";  // user + pass match
static constexpr const char* AUTH_PASS_ONLY = "T";   // pass match, wrong user
static constexpr const char* AUTH_FAIL      = "F";   // no match

// ─── Data model ───────────────────────────────────────────────────────────────

struct Credential {
    std::string username;
    std::string password;
};

// ─── Credential loading ───────────────────────────────────────────────────────

/**
 * @brief Load credentials from a CSV file (format: "username,password\n").
 *
 * Trailing whitespace on the password field is stripped automatically by
 * using std::ws after the comma.
 *
 * @param path Path to the credential file.
 * @return Vector of Credential structs; empty on error.
 */
static std::vector<Credential> loadCredentials(const std::string& path) {
    std::vector<Credential> creds;
    std::ifstream ifs(path);

    if (!ifs.is_open()) {
        std::cerr << "Error: could not open credential file: " << path << "\n";
        return creds;
    }

    Credential entry;
    while (std::getline(std::getline(ifs, entry.username, ',') >> std::ws,
                        entry.password)) {
        creds.push_back(entry);
    }

    return creds;
}

// ─── Authentication logic ─────────────────────────────────────────────────────

/**
 * @brief Check a username/password pair against the loaded credentials.
 *
 * @param creds    Credential list loaded from file.
 * @param username Received (possibly encoded) username.
 * @param password Received (possibly encoded) password.
 * @return One of: AUTH_OK, AUTH_PASS_ONLY, or AUTH_FAIL.
 */
static const char* authenticate(const std::vector<Credential>& creds,
                                const std::string& username,
                                const std::string& password) {
    for (const auto& c : creds) {
        if (c.password == password) {
            return (c.username == username) ? AUTH_OK : AUTH_PASS_ONLY;
        }
    }
    return AUTH_FAIL;
}

// ─── Socket initialisation ────────────────────────────────────────────────────

/**
 * @brief Create and bind a UDP socket on AUTH_UDP_PORT.
 * @return Bound socket file descriptor, or -1 on failure.
 */
static int createAndBind() {
    int fd = ::socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        ::perror("socket");
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(AUTH_UDP_PORT);
    addr.sin_addr.s_addr = ::inet_addr(SERVER_IP);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::perror("bind");
        ::close(fd);
        return -1;
    }

    return fd;
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main() {
    int fd = createAndBind();
    if (fd == -1) return 1;

    std::cout << "The ServerC is up and running using UDP on port "
              << AUTH_UDP_PORT << ".\n";

    // Pre-configure the main server's address (reply target)
    sockaddr_in mainAddr{};
    mainAddr.sin_family      = AF_INET;
    mainAddr.sin_port        = htons(MAIN_UDP_PORT);
    mainAddr.sin_addr.s_addr = ::inet_addr(SERVER_IP);

    char userBuf[RECV_BUF_SIZE];
    char passBuf[RECV_BUF_SIZE];

    while (true) {
        sockaddr_in senderAddr{};
        socklen_t   senderLen = sizeof(senderAddr);

        // ── Receive username ──────────────────────────────────────────────────
        ssize_t n = ::recvfrom(fd, userBuf, RECV_BUF_SIZE - 1, 0,
                               reinterpret_cast<sockaddr*>(&senderAddr),
                               &senderLen);
        if (n == -1) {
            ::perror("recvfrom (username)");
            break;
        }
        userBuf[n] = '\0';

        // ── Receive password ──────────────────────────────────────────────────
        n = ::recvfrom(fd, passBuf, RECV_BUF_SIZE - 1, 0,
                       reinterpret_cast<sockaddr*>(&senderAddr),
                       &senderLen);
        if (n == -1) {
            ::perror("recvfrom (password)");
            break;
        }
        passBuf[n] = '\0';

        std::cout << "The ServerC received an authentication request "
                     "from the Main Server.\n";

        // ── Load credentials & authenticate ───────────────────────────────────
        std::vector<Credential> creds = loadCredentials(CREDENTIAL_FILE);
        const char* result = authenticate(creds, userBuf, passBuf);

        // ── Reply to main server ──────────────────────────────────────────────
        if (::sendto(fd, result, std::strlen(result), 0,
                     reinterpret_cast<sockaddr*>(&mainAddr),
                     sizeof(mainAddr)) == -1) {
            ::perror("sendto");
            break;
        }

        std::cout << "The ServerC finished sending the response to the Main Server.\n\n";
    }

    ::close(fd);
    return 0;
}
