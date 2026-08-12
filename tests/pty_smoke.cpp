#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <pty.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int master = -1;
    const pid_t child = ::forkpty(&master, nullptr, nullptr, nullptr);
    if (child < 0) {
        std::cerr << "forkpty: " << std::strerror(errno) << '\n';
        return 1;
    }

    if (child == 0) {
        ::execl("/bin/sh", "sh", "-lc", "printf 'PTY_OK\\n'; pwd", nullptr);
        _exit(127);
    }

    std::string output;
    std::array<char, 1024> buffer{};
    for (;;) {
        const ssize_t count = ::read(master, buffer.data(), buffer.size());
        if (count > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(count));
            continue;
        }
        break;
    }

    ::close(master);
    int status = 0;
    ::waitpid(child, &status, 0);

    std::cout << output;
    return output.find("PTY_OK") == std::string::npos ? 2 : 0;
}
