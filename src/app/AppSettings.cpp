#include "AppSettings.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <algorithm>
#include <utility>

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

QStringList builtInProfiles()
{
    return {QStringLiteral("Default"), QStringLiteral("Development"), QStringLiteral("Server")};
}

QStringList builtInColorSchemes()
{
    return {
        QStringLiteral("Axiom Dark"),
        QStringLiteral("Midnight"),
        QStringLiteral("Graphite"),
        QStringLiteral("Forest"),
    };
}
}

AppSettings::AppSettings(QObject* parent)
    : QObject(parent)
    , m_settings(settingsFilePath(), QSettings::IniFormat)
    , m_defaultShell(detectedShell())
    , m_startDirectory(QDir::homePath())
{
    load();
}

QString AppSettings::terminalFontFamily() const { return m_terminalFontFamily; }
int AppSettings::terminalFontSize() const noexcept { return m_terminalFontSize; }
QString AppSettings::defaultShell() const { return m_defaultShell; }
QString AppSettings::startDirectory() const { return m_startDirectory; }
bool AppSettings::confirmCloseRunningProcesses() const noexcept { return m_confirmCloseRunningProcesses; }
QStringList AppSettings::profileNames() const { return m_profileNames; }
QString AppSettings::activeProfile() const { return m_activeProfile; }
QStringList AppSettings::colorSchemeNames() const { return builtInColorSchemes(); }

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
    const bool changed = m_defaultShell != normalized;
    m_defaultShell = normalized;
    persist(QStringLiteral("session/defaultShell"), m_defaultShell);

    if (m_profiles.contains(QStringLiteral("Default"))) {
        Profile& profile = m_profiles[QStringLiteral("Default")];
        if (profile.shell != normalized) {
            profile.shell = normalized;
            persistProfile(QStringLiteral("Default"));
            emit profilesChanged();
        }
    }

    if (changed) {
        emit defaultShellChanged();
    }
}

void AppSettings::setStartDirectory(const QString& directory)
{
    const QString normalized = normalizedStartDirectory(directory);
    const bool changed = m_startDirectory != normalized;
    m_startDirectory = normalized;
    persist(QStringLiteral("session/startDirectory"), m_startDirectory);

    if (m_profiles.contains(QStringLiteral("Default"))) {
        Profile& profile = m_profiles[QStringLiteral("Default")];
        if (profile.startDirectory != normalized) {
            profile.startDirectory = normalized;
            persistProfile(QStringLiteral("Default"));
            emit profilesChanged();
        }
    }

    if (changed) {
        emit startDirectoryChanged();
    }
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

void AppSettings::setActiveProfile(const QString& name)
{
    const QString normalized = validatedProfileName(name);
    if (normalized.isEmpty() || m_activeProfile == normalized) {
        return;
    }
    m_activeProfile = normalized;
    persist(QStringLiteral("profiles/active"), m_activeProfile);
    emit activeProfileChanged();
}

QString AppSettings::profileShell(const QString& name) const
{
    const QString normalized = validatedProfileName(name);
    return normalized.isEmpty() ? m_defaultShell : m_profiles.value(normalized).shell;
}

QString AppSettings::profileStartDirectory(const QString& name) const
{
    const QString normalized = validatedProfileName(name);
    return normalized.isEmpty() ? m_startDirectory : m_profiles.value(normalized).startDirectory;
}

QString AppSettings::profileColorScheme(const QString& name) const
{
    const QString normalized = validatedProfileName(name);
    return normalized.isEmpty() ? QStringLiteral("Axiom Dark") : m_profiles.value(normalized).colorScheme;
}

bool AppSettings::profileRemovable(const QString& name) const
{
    return m_profiles.contains(name) && !builtInProfiles().contains(name);
}

bool AppSettings::createProfile(const QString& name)
{
    const QString normalized = name.trimmed();
    if (normalized.isEmpty() || normalized.size() > 48 || normalized.contains(QLatin1Char('/'))
        || normalized.contains(QLatin1Char('\\')) || m_profiles.contains(normalized)) {
        return false;
    }

    Profile profile;
    profile.shell = m_defaultShell;
    profile.startDirectory = m_startDirectory;
    profile.colorScheme = QStringLiteral("Axiom Dark");
    m_profiles.insert(normalized, profile);
    m_profileNames.push_back(normalized);
    persistProfileNames();
    persistProfile(normalized);
    emit profilesChanged();
    return true;
}

bool AppSettings::removeProfile(const QString& name)
{
    if (!profileRemovable(name)) {
        return false;
    }

    const QString storageKey = profileStorageKey(name);
    m_profiles.remove(name);
    m_profileNames.removeAll(name);
    queueRemove(QStringLiteral("profiles/data/%1").arg(storageKey));
    persistProfileNames();

    if (m_activeProfile == name) {
        m_activeProfile = QStringLiteral("Default");
        persist(QStringLiteral("profiles/active"), m_activeProfile);
        emit activeProfileChanged();
    }
    emit profilesChanged();
    return true;
}

void AppSettings::setProfileShell(const QString& name, const QString& shellPath)
{
    const QString normalizedName = validatedProfileName(name);
    if (normalizedName.isEmpty()) {
        return;
    }

    const QString normalized = normalizedShell(shellPath);
    Profile& profile = m_profiles[normalizedName];
    if (profile.shell == normalized) {
        return;
    }
    profile.shell = normalized;
    persistProfile(normalizedName);

    if (normalizedName == QStringLiteral("Default")) {
        m_defaultShell = normalized;
        persist(QStringLiteral("session/defaultShell"), m_defaultShell);
        emit defaultShellChanged();
    }
}

void AppSettings::setProfileStartDirectory(const QString& name, const QString& directory)
{
    const QString normalizedName = validatedProfileName(name);
    if (normalizedName.isEmpty()) {
        return;
    }

    const QString normalized = normalizedStartDirectory(directory);
    Profile& profile = m_profiles[normalizedName];
    if (profile.startDirectory == normalized) {
        return;
    }
    profile.startDirectory = normalized;
    persistProfile(normalizedName);

    if (normalizedName == QStringLiteral("Default")) {
        m_startDirectory = normalized;
        persist(QStringLiteral("session/startDirectory"), m_startDirectory);
        emit startDirectoryChanged();
    }
}

void AppSettings::setProfileColorScheme(const QString& name, const QString& colorScheme)
{
    const QString normalizedName = validatedProfileName(name);
    if (normalizedName.isEmpty()) {
        return;
    }

    const QString normalizedScheme = normalizedColorScheme(colorScheme);
    Profile& profile = m_profiles[normalizedName];
    if (profile.colorScheme == normalizedScheme) {
        return;
    }
    profile.colorScheme = normalizedScheme;
    persistProfile(normalizedName);
    emit profileColorSchemeChanged(normalizedName, normalizedScheme);
}

void AppSettings::setProfileSettings(const QString& name, const QString& shellPath,
                                     const QString& directory, const QString& colorScheme)
{
    const QString normalizedName = validatedProfileName(name);
    if (normalizedName.isEmpty()) {
        return;
    }

    const QString normalizedProfileShell = normalizedShell(shellPath);
    const QString normalizedDirectory = normalizedStartDirectory(directory);
    const QString normalizedScheme = normalizedColorScheme(colorScheme);
    Profile& profile = m_profiles[normalizedName];

    const bool shellChanged = profile.shell != normalizedProfileShell;
    const bool directoryChanged = profile.startDirectory != normalizedDirectory;
    const bool schemeChanged = profile.colorScheme != normalizedScheme;
    if (!shellChanged && !directoryChanged && !schemeChanged) {
        return;
    }

    profile.shell = normalizedProfileShell;
    profile.startDirectory = normalizedDirectory;
    profile.colorScheme = normalizedScheme;
    persistProfile(normalizedName);

    if (normalizedName == QStringLiteral("Default")) {
        if (shellChanged) {
            m_defaultShell = normalizedProfileShell;
            persist(QStringLiteral("session/defaultShell"), m_defaultShell);
            emit defaultShellChanged();
        }
        if (directoryChanged) {
            m_startDirectory = normalizedDirectory;
            persist(QStringLiteral("session/startDirectory"), m_startDirectory);
            emit startDirectoryChanged();
        }
    }

    if (schemeChanged) {
        emit profileColorSchemeChanged(normalizedName, normalizedScheme);
    }
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
    const bool confirmChanged = !m_confirmCloseRunningProcesses;
    const bool activeProfileChangedValue = m_activeProfile != QStringLiteral("Default");

    m_terminalFontFamily = defaultFontFamily;
    m_terminalFontSize = DefaultTerminalFontSize;
    m_defaultShell = defaultShell;
    m_startDirectory = defaultDirectory;
    m_confirmCloseRunningProcesses = true;
    resetProfilesToDefaults();
    m_activeProfile = QStringLiteral("Default");

    m_pendingWrites.clear();
    m_pendingRemovals.clear();
    m_pendingClear = true;
    persistProfileNames();
    for (const QString& profileName : std::as_const(m_profileNames)) {
        persistProfile(profileName);
    }
    persist(QStringLiteral("profiles/active"), m_activeProfile);

    if (fontFamilyChanged) emit terminalFontFamilyChanged();
    if (fontSizeChanged) emit terminalFontSizeChanged();
    if (shellChanged) emit defaultShellChanged();
    if (directoryChanged) emit startDirectoryChanged();
    if (confirmChanged) emit confirmCloseRunningProcessesChanged();
    emit profilesChanged();
    if (activeProfileChangedValue) emit activeProfileChanged();
    emit defaultsReset();
}

void AppSettings::flush()
{
    if (m_pendingClear) {
        m_settings.clear();
        m_pendingClear = false;
    }

    for (const QString& prefix : std::as_const(m_pendingRemovals)) {
        m_settings.remove(prefix);
    }
    m_pendingRemovals.clear();

    for (auto it = m_pendingWrites.cbegin(); it != m_pendingWrites.cend(); ++it) {
        m_settings.setValue(it.key(), it.value());
    }
    m_pendingWrites.clear();

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
    m_confirmCloseRunningProcesses = m_settings.value(QStringLiteral("session/confirmCloseRunningProcesses"), true).toBool();

    loadProfiles();
    const QString configuredActive = m_settings.value(QStringLiteral("profiles/active"), QStringLiteral("Default")).toString();
    m_activeProfile = m_profiles.contains(configuredActive) ? configuredActive : QStringLiteral("Default");
}

void AppSettings::persist(const QString& key, const QVariant& value)
{
    // Keep Settings mutations entirely in-memory while the user is interacting
    // with the UI. Disk I/O is batched and performed only by flush(), normally
    // when AxiomTTY is shutting down (or explicitly by tests).
    m_pendingWrites.insert(key, value);
}

void AppSettings::queueRemove(const QString& keyPrefix)
{
    if (!m_pendingRemovals.contains(keyPrefix)) {
        m_pendingRemovals.push_back(keyPrefix);
    }

    for (auto it = m_pendingWrites.begin(); it != m_pendingWrites.end();) {
        if (it.key() == keyPrefix || it.key().startsWith(keyPrefix + QLatin1Char('/'))) {
            it = m_pendingWrites.erase(it);
        } else {
            ++it;
        }
    }
}

void AppSettings::resetProfilesToDefaults()
{
    m_profiles.clear();
    m_profileNames = builtInProfiles();
    for (const QString& name : std::as_const(m_profileNames)) {
        m_profiles.insert(name, defaultProfileForName(name));
    }
}

void AppSettings::loadProfiles()
{
    resetProfilesToDefaults();

    const QStringList storedNames = m_settings.value(QStringLiteral("profiles/names")).toStringList();
    for (const QString& storedName : storedNames) {
        const QString name = storedName.trimmed();
        if (!name.isEmpty() && !m_profileNames.contains(name)) {
            m_profileNames.push_back(name);
            m_profiles.insert(name, defaultProfileForName(name));
        }
    }

    for (const QString& name : std::as_const(m_profileNames)) {
        Profile profile = m_profiles.value(name);
        const QString root = QStringLiteral("profiles/data/%1/").arg(profileStorageKey(name));
        profile.shell = normalizedShell(m_settings.value(root + QStringLiteral("shell"), profile.shell).toString());
        profile.startDirectory = normalizedStartDirectory(
            m_settings.value(root + QStringLiteral("startDirectory"), profile.startDirectory).toString());
        profile.colorScheme = normalizedColorScheme(
            m_settings.value(root + QStringLiteral("colorScheme"), profile.colorScheme).toString());
        m_profiles.insert(name, profile);
    }

    const Profile defaultProfile = m_profiles.value(QStringLiteral("Default"));
    m_defaultShell = defaultProfile.shell;
    m_startDirectory = defaultProfile.startDirectory;
}

void AppSettings::persistProfileNames()
{
    persist(QStringLiteral("profiles/names"), m_profileNames);
}

void AppSettings::persistProfile(const QString& name)
{
    if (!m_profiles.contains(name)) {
        return;
    }
    const Profile profile = m_profiles.value(name);
    const QString root = QStringLiteral("profiles/data/%1/").arg(profileStorageKey(name));
    persist(root + QStringLiteral("shell"), profile.shell);
    persist(root + QStringLiteral("startDirectory"), profile.startDirectory);
    persist(root + QStringLiteral("colorScheme"), profile.colorScheme);
}

QString AppSettings::validatedProfileName(const QString& name) const
{
    const QString normalized = name.trimmed();
    return m_profiles.contains(normalized) ? normalized : QString{};
}

QString AppSettings::profileStorageKey(const QString& name) const
{
    return QString::fromLatin1(name.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString AppSettings::normalizedColorScheme(const QString& colorScheme) const
{
    const QString normalized = colorScheme.trimmed();
    return builtInColorSchemes().contains(normalized) ? normalized : QStringLiteral("Axiom Dark");
}

AppSettings::Profile AppSettings::defaultProfileForName(const QString& name) const
{
    Profile profile;
    profile.shell = m_defaultShell.isEmpty() ? detectedShell() : m_defaultShell;
    profile.startDirectory = m_startDirectory.isEmpty() ? QDir::homePath() : m_startDirectory;
    profile.colorScheme = QStringLiteral("Axiom Dark");

    if (name == QStringLiteral("Development")) {
        profile.colorScheme = QStringLiteral("Midnight");
    } else if (name == QStringLiteral("Server")) {
        profile.colorScheme = QStringLiteral("Graphite");
    }
    return profile;
}
