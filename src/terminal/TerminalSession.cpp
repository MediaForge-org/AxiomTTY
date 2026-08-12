#include "TerminalSession.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcessEnvironment>

TerminalSession::TerminalSession(QObject* parent)
    : QObject(parent)
    , m_screen(30, 120)
    , m_parser(m_screen)
{
    m_parser.setResponseHandler([this](const QByteArray& bytes) { writeParserResponse(bytes); });
    m_parser.setTitleHandler([this](const QString& title) {
        if (!title.isEmpty()) {
            setTitle(title);
        }
    });

    connect(&m_process, &PtyProcess::outputReady, this, &TerminalSession::consumeOutput);

    connect(&m_process, &PtyProcess::started, this, [this](qint64) {
        emit runningChanged();
        emit processIdChanged();
    });

    connect(&m_process, &PtyProcess::finished, this, [this](int exitCode, int termSignal) {
        const QString message = QStringLiteral("\r\n[process ended: exit=%1 signal=%2]\r\n")
                                    .arg(exitCode)
                                    .arg(termSignal);
        m_parser.consume(message.toUtf8());
        emit screenChanged();
        emit runningChanged();
        emit processIdChanged();
    });

    connect(&m_process, &PtyProcess::errorOccurred, this, [this](const QString& message) {
        m_parser.consume(QStringLiteral("\r\n[terminal error] %1\r\n").arg(message).toUtf8());
        emit screenChanged();
    });
}

QString TerminalSession::title() const { return m_title; }
QString TerminalSession::shell() const { return m_shell; }
bool TerminalSession::running() const { return m_process.isRunning(); }
qint64 TerminalSession::processId() const { return m_process.processId(); }
bool TerminalSession::applicationCursorKeys() const { return m_screen.applicationCursorKeys(); }
bool TerminalSession::bracketedPaste() const { return m_screen.bracketedPaste(); }
int TerminalSession::rows() const noexcept { return m_screen.rows(); }
int TerminalSession::columns() const noexcept { return m_screen.columns(); }
QString TerminalSession::workingDirectory() const { return m_workingDirectory; }
const TerminalScreen& TerminalSession::screen() const noexcept { return m_screen; }

void TerminalSession::startDefaultShell()
{
    startDefaultShellInDirectory(m_workingDirectory.isEmpty() ? QDir::homePath() : m_workingDirectory);
}

void TerminalSession::startDefaultShellInDirectory(const QString& workingDirectory)
{
    const QString configuredShell = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL"));
    const QString candidate = configuredShell.isEmpty() ? QStringLiteral("/bin/bash") : configuredShell;
    startShellInDirectory(candidate, workingDirectory);
}

void TerminalSession::startShell(const QString& shellPath)
{
    startShellInDirectory(shellPath, m_workingDirectory.isEmpty() ? QDir::homePath() : m_workingDirectory);
}

void TerminalSession::startShellInDirectory(const QString& shellPath, const QString& workingDirectory)
{
    if (m_process.isRunning()) {
        return;
    }

    m_screen.reset();
    m_parser.reset();
    emit screenChanged();
    emit terminalModesChanged();

    m_shell = shellPath;
    emit shellChanged();

    const QString startDirectory = workingDirectory.isEmpty() ? QDir::homePath() : workingDirectory;
    if (m_workingDirectory != startDirectory) {
        m_workingDirectory = startDirectory;
        emit workingDirectoryChanged();
    }

    const QFileInfo shellInfo(shellPath);
    setTitle(shellInfo.fileName().isEmpty() ? QStringLiteral("Shell") : shellInfo.fileName());

    QStringList arguments;
    const QString executableName = shellInfo.fileName();
    if (executableName == QStringLiteral("bash")
        || executableName == QStringLiteral("zsh")
        || executableName == QStringLiteral("fish")) {
        arguments << QStringLiteral("-i");
    }

    if (!m_process.start(shellPath, arguments, startDirectory)) {
        m_parser.consume(QStringLiteral("[failed to start %1]\r\n").arg(shellPath).toUtf8());
        emit screenChanged();
    } else {
        m_process.resize(m_screen.rows(), m_screen.columns());
        refreshWorkingDirectory();
    }
}

void TerminalSession::sendText(const QString& text)
{
    if (m_process.isRunning() && !text.isEmpty()) {
        m_process.write(text.toUtf8());
    }
}

void TerminalSession::sendBytes(const QByteArray& bytes)
{
    if (m_process.isRunning() && !bytes.isEmpty()) {
        m_process.write(bytes);
    }
}

void TerminalSession::pasteClipboard()
{
    if (!m_process.isRunning()) {
        return;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        return;
    }

    const QByteArray payload = clipboard->text(QClipboard::Clipboard).toUtf8();
    if (payload.isEmpty()) {
        return;
    }

    if (m_screen.bracketedPaste()) {
        m_process.write(QByteArray("\x1b[200~") + payload + QByteArray("\x1b[201~"));
    } else {
        m_process.write(payload);
    }
}

void TerminalSession::sendInterrupt()
{
    m_process.write(QByteArray(1, '\x03'));
}

void TerminalSession::sendSuspend()
{
    m_process.write(QByteArray(1, '\x1A'));
}

void TerminalSession::clearDisplay()
{
    m_screen.eraseDisplay(2);
    m_screen.setCursorPosition(0, 0);
    emit screenChanged();
}

void TerminalSession::resizeTerminal(int rowCount, int columnCount)
{
    if (rowCount <= 0 || columnCount <= 0) {
        return;
    }
    if (rowCount == m_screen.rows() && columnCount == m_screen.columns()) {
        return;
    }

    m_screen.resize(rowCount, columnCount);
    m_process.resize(rowCount, columnCount);
    emit screenChanged();
    emit terminalSizeChanged();
}


void TerminalSession::refreshWorkingDirectory()
{
    if (!m_process.isRunning() || m_process.processId() <= 0) {
        return;
    }

    const QString procLink = QStringLiteral("/proc/%1/cwd").arg(m_process.processId());
    const QString target = QFileInfo(procLink).symLinkTarget();
    if (target.isEmpty() || target == m_workingDirectory) {
        return;
    }

    m_workingDirectory = target;
    emit workingDirectoryChanged();
}

void TerminalSession::setInitialWorkingDirectory(const QString& workingDirectory)
{
    if (m_process.isRunning()) {
        return;
    }

    const QString normalized = workingDirectory.isEmpty() ? QDir::homePath() : workingDirectory;
    if (m_workingDirectory == normalized) {
        return;
    }
    m_workingDirectory = normalized;
    emit workingDirectoryChanged();
}

void TerminalSession::consumeOutput(const QByteArray& bytes)
{
    const bool oldApplicationCursorKeys = m_screen.applicationCursorKeys();
    const bool oldBracketedPaste = m_screen.bracketedPaste();
    const bool oldSynchronizedOutput = m_screen.synchronizedOutput();

    m_parser.consume(bytes);

    // DEC private mode 2026 lets TUIs submit a batch of screen changes without
    // exposing every intermediate frame. While synchronization is active we
    // keep updating the model but defer the expensive GUI repaint until the
    // application ends the synchronized update.
    if (!m_screen.synchronizedOutput()) {
        emit screenChanged();
    }

    if (oldApplicationCursorKeys != m_screen.applicationCursorKeys()
        || oldBracketedPaste != m_screen.bracketedPaste()
        || oldSynchronizedOutput != m_screen.synchronizedOutput()) {
        emit terminalModesChanged();
    }
}

void TerminalSession::writeParserResponse(const QByteArray& bytes)
{
    if (m_process.isRunning() && !bytes.isEmpty()) {
        m_process.write(bytes);
    }
}

void TerminalSession::setTitle(const QString& titleValue)
{
    if (m_title == titleValue) {
        return;
    }
    m_title = titleValue;
    emit titleChanged();
}
