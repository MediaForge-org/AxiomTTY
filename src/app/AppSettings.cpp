#include "AppSettings.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QFileInfo>

#include <algorithm>

namespace {
QString detectedShell()
{
    const QString shell = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL")).trimmed();
    const QString candidate = shell.isEmpty() ? QStringLiteral("/bin/bash") : shell;
    const QFileInfo info(candidate);
    return info.isFile() && info.isExecutable() ? info.absoluteFilePath() : QStringLiteral("/bin/bash");
}

QString normalizedShell(const QString& shellPath)
{
    const QString candidate = shellPath.trimmed();
    if (!candidate.isEmpty()) {
        const QFileInfo info(candidate);
        if (info.isFile() && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }
    return detectedShell();
}

QString normalizedStartDirectory(const QString& directory)
{
    QString candidate = directory.trimmed();
    if (candidate.isEmpty() || candidate == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (candidate.startsWith(QStringLiteral("~/"))) {
        candidate = QDir::homePath() + candidate.mid(1);
    }
    const QFileInfo info(candidate);
    return info.isDir() ? QDir(candidate).absolutePath() : QDir::homePath();
}

QString settingsFilePath()
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir directory(configRoot);
    directory.mkpath(QStringLiteral("axiomtty"));
    return directory.filePath(QStringLiteral("axiomtty/settings.ini"));
}
}

AppSettings::AppSettings(QObject* parent)
    : QObject(parent)
    , m_settings(settingsFilePath(), QSettings::IniFormat)
    , m_defaultShell(detectedShell())
    , m_startDirectory(QDir::homePath())
{
    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(120);
    connect(&m_syncTimer, &QTimer::timeout, this, &AppSettings::flush);
    load();
}

QString AppSettings::terminalFontFamily() const { return m_terminalFontFamily; }
int AppSettings::terminalFontSize() const noexcept { return m_terminalFontSize; }
QString AppSettings::defaultShell() const { return m_defaultShell; }
QString AppSettings::startDirectory() const { return m_startDirectory; }
bool AppSettings::inheritWorkingDirectory() const noexcept { return m_inheritWorkingDirectory; }
bool AppSettings::confirmCloseRunningProcesses() const noexcept { return m_confirmCloseRunningProcesses; }

void AppSettings::setTerminalFontFamily(const QString& family)
{
    const QString normalized = family.trimmed().isEmpty() ? QStringLiteral("monospace") : family.trimmed();
    if (m_terminalFontFamily == normalized) {
        return;
    }
    m_terminalFontFamily = normalized;
    persist(QStringLiteral("terminal/fontFamily"), m_terminalFontFamily);
    emit terminalFontFamilyChanged();
}

void AppSettings::setTerminalFontSize(int size)
{
    const int clamped = std::clamp(size, 8, 48);
    if (m_terminalFontSize == clamped) {
        return;
    }
    m_terminalFontSize = clamped;
    persist(QStringLiteral("terminal/fontSize"), m_terminalFontSize);
    emit terminalFontSizeChanged();
}

void AppSettings::setDefaultShell(const QString& shellPath)
{
    const QString normalized = normalizedShell(shellPath);
    if (m_defaultShell == normalized) {
        return;
    }
    m_defaultShell = normalized;
    persist(QStringLiteral("session/defaultShell"), m_defaultShell);
    emit defaultShellChanged();
}

void AppSettings::setStartDirectory(const QString& directory)
{
    const QString normalized = normalizedStartDirectory(directory);
    if (m_startDirectory == normalized) {
        return;
    }
    m_startDirectory = normalized;
    persist(QStringLiteral("session/startDirectory"), m_startDirectory);
    emit startDirectoryChanged();
}

void AppSettings::setInheritWorkingDirectory(bool enabled)
{
    if (m_inheritWorkingDirectory == enabled) {
        return;
    }
    m_inheritWorkingDirectory = enabled;
    persist(QStringLiteral("session/inheritWorkingDirectory"), enabled);
    emit inheritWorkingDirectoryChanged();
}

void AppSettings::setConfirmCloseRunningProcesses(bool enabled)
{
    if (m_confirmCloseRunningProcesses == enabled) {
        return;
    }
    m_confirmCloseRunningProcesses = enabled;
    persist(QStringLiteral("session/confirmCloseRunningProcesses"), enabled);
    emit confirmCloseRunningProcessesChanged();
}

void AppSettings::resetDefaults()
{
    const QString defaultFontFamily = QStringLiteral("monospace");
    const QString defaultShell = detectedShell();
    const QString defaultDirectory = QDir::homePath();

    const bool fontFamilyChanged = m_terminalFontFamily != defaultFontFamily;
    const bool fontSizeChanged = m_terminalFontSize != DefaultTerminalFontSize;
    const bool shellChanged = m_defaultShell != defaultShell;
    const bool directoryChanged = m_startDirectory != defaultDirectory;
    const bool inheritChanged = !m_inheritWorkingDirectory;
    const bool confirmChanged = !m_confirmCloseRunningProcesses;

    m_terminalFontFamily = defaultFontFamily;
    m_terminalFontSize = DefaultTerminalFontSize;
    m_defaultShell = defaultShell;
    m_startDirectory = defaultDirectory;
    m_inheritWorkingDirectory = true;
    m_confirmCloseRunningProcesses = true;

    m_settings.clear();
    scheduleSync();

    if (fontFamilyChanged) emit terminalFontFamilyChanged();
    if (fontSizeChanged) emit terminalFontSizeChanged();
    if (shellChanged) emit defaultShellChanged();
    if (directoryChanged) emit startDirectoryChanged();
    if (inheritChanged) emit inheritWorkingDirectoryChanged();
    if (confirmChanged) emit confirmCloseRunningProcessesChanged();
    emit defaultsReset();
}

void AppSettings::flush()
{
    if (m_syncTimer.isActive()) {
        m_syncTimer.stop();
    }
    m_settings.sync();
}

void AppSettings::load()
{
    m_terminalFontFamily = m_settings.value(QStringLiteral("terminal/fontFamily"), m_terminalFontFamily).toString();
    m_terminalFontSize = std::clamp(
        m_settings.value(QStringLiteral("terminal/fontSize"), DefaultTerminalFontSize).toInt(), 8, 48);
    m_defaultShell = normalizedShell(
        m_settings.value(QStringLiteral("session/defaultShell"), m_defaultShell).toString());
    m_startDirectory = normalizedStartDirectory(
        m_settings.value(QStringLiteral("session/startDirectory"), m_startDirectory).toString());
    m_inheritWorkingDirectory = m_settings.value(QStringLiteral("session/inheritWorkingDirectory"), true).toBool();
    m_confirmCloseRunningProcesses = m_settings.value(QStringLiteral("session/confirmCloseRunningProcesses"), true).toBool();
}

void AppSettings::persist(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
    scheduleSync();
}

void AppSettings::scheduleSync()
{
    m_syncTimer.start();
}
