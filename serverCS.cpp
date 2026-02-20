/**
 * @file serverCS.cpp
 * @brief CS department course-query server (ServerCS).
 *
 * Listens on a UDP socket for (course_code, category) query pairs forwarded
 * by the main server, looks up the answer in "cs.txt", and replies with the
 * result (or a descriptive error message).
 *
 * cs.txt format (one course per line):
 *   code,credit,professor,days,courseName
 *
 * Category tokens accepted: Credit | Professor | Days | CourseName
 *
 * Build:  g++ -std=c++17 -Wall -Wextra -o serverCS serverCS.cpp
 * Usage:  ./serverCS
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─── Configuration ────────────────────────────────────────────────────────────

static constexpr const char* SERVER_IP      = "127.0.0.1";
static constexpr uint16_t    CS_UDP_PORT    = 22289;   // this server's port
static constexpr uint16_t    MAIN_UDP_PORT  = 24289;   // main server's port
static constexpr size_t      RECV_BUF_SIZE  = 40000;
static constexpr const char* COURSE_FILE    = "cs.txt";

// ─── Data model ───────────────────────────────────────────────────────────────

struct Course {
    std::string code;
    std::string credit;
    std::string professor;
    std::string days;
    std::string name;
};

// ─── Course loading ───────────────────────────────────────────────────────────

/**
 * @brief Load courses from a CSV file.
 *
 * Expected format per line: code,credit,professor,days,courseName
 *
 * @param path Path to the course data file.
 * @return Vector of Course structs; empty on error.
 */
static std::vector<Course> loadCourses(const std::string& path) {
    std::vector<Course> courses;
    std::ifstream ifs(path);

    if (!ifs.is_open()) {
        std::cerr << "Error: could not open course file: " << path << "\n";
        return courses;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        Course c;
        std::getline(ss, c.code,      ',');
        std::getline(ss, c.credit,    ',');
        std::getline(ss, c.professor, ',');
        std::getline(ss, c.days,      ',');
        std::getline(ss, c.name,      ',');
        courses.push_back(std::move(c));
    }

    return courses;
}

// ─── Query resolution ─────────────────────────────────────────────────────────

/**
 * @brief Look up a specific field for a given course code.
 *
 * @param courses  Loaded course list.
 * @param code     Course code to find.
 * @param category Field to retrieve: "Credit", "Professor", "Days", or "CourseName".
 * @return The requested field value, or an error message string.
 */
static std::string resolveQuery(const std::vector<Course>& courses,
                                const std::string& code,
                                const std::string& category) {
    for (const auto& c : courses) {
        if (c.code != code) continue;

        // Course found — now resolve the category
        if (category == "Credit")     return c.credit;
        if (category == "Professor")  return c.professor;
        if (category == "Days")       return c.days;
        if (category == "CourseName") return c.name;

        return "Error: unknown category \"" + category + "\"";
    }

    return "Didn't find the course: " + code;
}

// ─── Socket initialisation ────────────────────────────────────────────────────

/**
 * @brief Create and bind a UDP socket on CS_UDP_PORT.
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
    addr.sin_port        = htons(CS_UDP_PORT);
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

    std::cout << "The ServerCS is up and running using UDP on port "
              << CS_UDP_PORT << ".\n";

    // Pre-configure the main server's reply address
    sockaddr_in mainAddr{};
    mainAddr.sin_family      = AF_INET;
    mainAddr.sin_port        = htons(MAIN_UDP_PORT);
    mainAddr.sin_addr.s_addr = ::inet_addr(SERVER_IP);

    char codeBuf[RECV_BUF_SIZE];
    char categoryBuf[RECV_BUF_SIZE];

    while (true) {
        sockaddr_in senderAddr{};
        socklen_t   senderLen = sizeof(senderAddr);

        // ── Receive course code ───────────────────────────────────────────────
        ssize_t n = ::recvfrom(fd, codeBuf, RECV_BUF_SIZE - 1, 0,
                               reinterpret_cast<sockaddr*>(&senderAddr),
                               &senderLen);
        if (n == -1) {
            ::perror("recvfrom (course code)");
            break;
        }
        codeBuf[n] = '\0';

        // ── Receive category ──────────────────────────────────────────────────
        n = ::recvfrom(fd, categoryBuf, RECV_BUF_SIZE - 1, 0,
                       reinterpret_cast<sockaddr*>(&senderAddr),
                       &senderLen);
        if (n == -1) {
            ::perror("recvfrom (category)");
            break;
        }
        categoryBuf[n] = '\0';

        std::string code(codeBuf);
        std::string category(categoryBuf);

        std::cout << "The ServerCS received a request from the Main Server "
                     "about the " << category << " of " << code << ".\n";

        // ── Load courses and resolve the query ────────────────────────────────
        std::vector<Course> courses = loadCourses(COURSE_FILE);
        std::string result = resolveQuery(courses, code, category);

        std::cout << "The course information has been found: The "
                  << category << " of " << code << " is " << result << ".\n";

        // ── Send result to main server ────────────────────────────────────────
        if (::sendto(fd, result.data(), result.size(), 0,
                     reinterpret_cast<sockaddr*>(&mainAddr),
                     sizeof(mainAddr)) == -1) {
            ::perror("sendto");
            break;
        }

        std::cout << "The ServerCS finished sending the response to the Main Server.\n\n";
    }

    ::close(fd);
    return 0;
}
