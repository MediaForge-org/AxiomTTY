#include "PtyProcess.h"

#include <QSocketNotifier>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
QString errnoMessage(const char* operation)
{
    return QStringLiteral("%1: %2")
        .arg(QString::fromUtf8(operation), QString::fromLocal8Bit(std::strerror(errno)));
}
}

PtyProcess::PtyProcess(QObject* parent)
    : QObject(parent)
    , m_childPollTimer(std::make_unique<QTimer>())
{
    m_childPollTimer->setInterval(100);
    QObject::connect(m_childPollTimer.get(), &QTimer::timeout, this, &PtyProcess::checkChildState);
}

PtyProcess::~PtyProcess()
{
    terminate();
    closeMaster();
}

bool PtyProcess::isRunning() const noexcept
{
    return m_running;
}

qint64 PtyProcess::processId() const noexcept
{
    return m_pid;
}

bool PtyProcess::hasChildProcesses() const
{
    if (!m_running || m_pid <= 0) {
        return false;
    }

    const QString childrenPath = QStringLiteral("/proc/%1/task/%1/children").arg(m_pid);
    QFile childrenFile(childrenPath);
    if (!childrenFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    return !childrenFile.readAll().trimmed().isEmpty();
}

bool PtyProcess::start(
    const QString& program,
    const QStringList& arguments,
    const QString& workingDirectory)
{
    if (m_running) {
        emit errorOccurred(QStringLiteral("A PTY process is already running."));
        return false;
    }

    struct winsize windowSize {};
    windowSize.ws_row = 30;
    windowSize.ws_col = 120;

    const pid_t childPid = ::forkpty(&m_masterFd, nullptr, nullptr, &windowSize);
    if (childPid < 0) {
        emit errorOccurred(errnoMessage("forkpty"));
        m_masterFd = -1;
        return false;
    }

    if (childPid == 0) {
        if (!workingDirectory.isEmpty()) {
            const QByteArray path = QFile::encodeName(workingDirectory);
            if (::chdir(path.constData()) != 0) {
                const QByteArray message = QByteArray("chdir failed: ") + std::strerror(errno) + "\n";
                ::write(STDERR_FILENO, message.constData(), static_cast<size_t>(message.size()));
                _exit(126);
            }
        }

        ::setenv("TERM", "xterm-256color", 1);
        ::setenv("COLORTERM", "truecolor", 1);
        ::setenv("TERM_PROGRAM", "AxiomTTY", 1);

        std::vector<QByteArray> encodedArguments;
        encodedArguments.reserve(static_cast<size_t>(arguments.size()) + 1U);
        encodedArguments.push_back(QFile::encodeName(program));
        for (const QString& argument : arguments) {
            encodedArguments.push_back(argument.toLocal8Bit());
        }

        std::vector<char*> argv;
        argv.reserve(encodedArguments.size() + 1U);
        for (QByteArray& argument : encodedArguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);

        ::execvp(argv.front(), argv.data());

        const QByteArray message = QByteArray("execvp failed: ") + std::strerror(errno) + "\n";
        ::write(STDERR_FILENO, message.constData(), static_cast<size_t>(message.size()));
        _exit(127);
    }

    m_pid = static_cast<qint64>(childPid);
    m_running = true;

    const int currentFlags = ::fcntl(m_masterFd, F_GETFL, 0);
    if (currentFlags >= 0) {
        ::fcntl(m_masterFd, F_SETFL, currentFlags | O_NONBLOCK);
    }

    m_readNotifier = std::make_unique<QSocketNotifier>(m_masterFd, QSocketNotifier::Read);
    QObject::connect(
        m_readNotifier.get(),
        &QSocketNotifier::activated,
        this,
        [this](auto, auto) { readAvailable(); });

    m_childPollTimer->start();
    emit started(m_pid);
    return true;
}

qint64 PtyProcess::write(const QByteArray& data)
{
    if (!m_running || m_masterFd < 0 || data.isEmpty()) {
        return 0;
    }

    qint64 totalWritten = 0;
    while (totalWritten < data.size()) {
        const auto remaining = static_cast<size_t>(data.size() - totalWritten);
        const ssize_t result = ::write(m_masterFd, data.constData() + totalWritten, remaining);
        if (result > 0) {
            totalWritten += static_cast<qint64>(result);
            continue;
        }

        if (result < 0 && errno == EINTR) {
            continue;
        }

        if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }

        if (result < 0) {
            emit errorOccurred(errnoMessage("write"));
        }
        break;
    }

    return totalWritten;
}

void PtyProcess::resize(int rows, int columns)
{
    if (m_masterFd < 0 || rows <= 0 || columns <= 0) {
        return;
    }

    struct winsize windowSize {};
    windowSize.ws_row = static_cast<unsigned short>(rows);
    windowSize.ws_col = static_cast<unsigned short>(columns);

    if (::ioctl(m_masterFd, TIOCSWINSZ, &windowSize) == -1) {
        emit errorOccurred(errnoMessage("ioctl(TIOCSWINSZ)"));
        return;
    }

    if (m_pid > 0) {
        ::kill(static_cast<pid_t>(m_pid), SIGWINCH);
    }
}

void PtyProcess::sendSignal(int signalNumber)
{
    if (!m_running || m_pid <= 0) {
        return;
    }

    // forkpty creates the child as the process-group leader. Signalling the
    // process group reaches foreground descendants such as sudo, vim or ssh.
    if (::kill(-static_cast<pid_t>(m_pid), signalNumber) == -1 && errno != ESRCH) {
        emit errorOccurred(errnoMessage("kill"));
    }
}

void PtyProcess::terminate()
{
    if (!m_running || m_pid <= 0) {
        return;
    }

    sendSignal(SIGHUP);
}

void PtyProcess::readAvailable()
{
    if (m_masterFd < 0) {
        return;
    }

    QByteArray collected;
    char buffer[8192];

    for (;;) {
        const ssize_t count = ::read(m_masterFd, buffer, sizeof(buffer));
        if (count > 0) {
            collected.append(buffer, static_cast<qsizetype>(count));
            continue;
        }

        if (count < 0 && errno == EINTR) {
            continue;
        }

        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }

        // EIO is a normal Linux PTY result after the slave side closes.
        if (count < 0 && errno != EIO) {
            emit errorOccurred(errnoMessage("read"));
        }
        break;
    }

    if (!collected.isEmpty()) {
        emit outputReady(collected);
    }
}

void PtyProcess::checkChildState()
{
    if (!m_running || m_pid <= 0) {
        return;
    }

    int status = 0;
    const pid_t result = ::waitpid(static_cast<pid_t>(m_pid), &status, WNOHANG);
    if (result == 0) {
        return;
    }

    if (result < 0) {
        if (errno != ECHILD) {
            emit errorOccurred(errnoMessage("waitpid"));
        }
        return;
    }

    readAvailable();
    m_childPollTimer->stop();
    m_running = false;

    int exitCode = -1;
    int termSignal = 0;
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        termSignal = WTERMSIG(status);
    }

    m_pid = -1;
    closeMaster();
    emit finished(exitCode, termSignal);
}

void PtyProcess::closeMaster()
{
    m_readNotifier.reset();
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
}
