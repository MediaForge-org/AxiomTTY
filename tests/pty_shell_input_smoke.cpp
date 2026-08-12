#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    int masterFd = -1;
    const pid_t pid = ::forkpty(&masterFd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        std::cerr << "forkpty failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    if (pid == 0) {
        ::execl("/bin/bash", "bash", "--noprofile", "--norc", "-i", static_cast<char*>(nullptr));
        _exit(127);
    }

    const int flags = ::fcntl(masterFd, F_GETFL, 0);
    ::fcntl(masterFd, F_SETFL, flags | O_NONBLOCK);

    const std::string command = "printf '__TERMINALCPP_INPUT_OK__\\n'\r";
    if (::write(masterFd, command.data(), command.size()) < 0) {
        std::cerr << "write failed: " << std::strerror(errno) << '\n';
        return 2;
    }

    std::string output;
    std::array<char, 4096> buffer{};
    bool found = false;

    for (int attempt = 0; attempt < 100 && !found; ++attempt) {
        const ssize_t count = ::read(masterFd, buffer.data(), buffer.size());
        if (count > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(count));
            found = output.find("__TERMINALCPP_INPUT_OK__") != std::string::npos;
        } else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            break;
        }
        ::usleep(10'000);
    }

    const std::string exitCommand = "exit\r";
    ::write(masterFd, exitCommand.data(), exitCommand.size());

    int status = 0;
    ::waitpid(pid, &status, 0);
    ::close(masterFd);

    if (!found) {
        std::cerr << "shell did not execute PTY input. Output was:\n" << output << '\n';
        return 3;
    }

    std::cout << "PTY shell input OK\n";
    return 0;
}
