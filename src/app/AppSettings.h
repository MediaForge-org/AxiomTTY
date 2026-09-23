#pragma once

#include <QHash>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

class AppSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString terminalFontFamily READ terminalFontFamily WRITE setTerminalFontFamily NOTIFY terminalFontFamilyChanged)
    Q_PROPERTY(int terminalFontSize READ terminalFontSize WRITE setTerminalFontSize NOTIFY terminalFontSizeChanged)
    Q_PROPERTY(QString defaultShell READ defaultShell WRITE setDefaultShell NOTIFY defaultShellChanged)
    Q_PROPERTY(QString startDirectory READ startDirectory WRITE setStartDirectory NOTIFY startDirectoryChanged)
    Q_PROPERTY(bool confirmCloseRunningProcesses READ confirmCloseRunningProcesses WRITE setConfirmCloseRunningProcesses NOTIFY confirmCloseRunningProcessesChanged)
    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profilesChanged)
    Q_PROPERTY(QString activeProfile READ activeProfile WRITE setActiveProfile NOTIFY activeProfileChanged)
    Q_PROPERTY(QStringList colorSchemeNames READ colorSchemeNames CONSTANT)

public:
    explicit AppSettings(QObject* parent = nullptr);

    [[nodiscard]] QString terminalFontFamily() const;
    [[nodiscard]] int terminalFontSize() const noexcept;
    [[nodiscard]] QString defaultShell() const;
    [[nodiscard]] QString startDirectory() const;
    [[nodiscard]] bool confirmCloseRunningProcesses() const noexcept;
    [[nodiscard]] QStringList profileNames() const;
    [[nodiscard]] QString activeProfile() const;
    [[nodiscard]] QStringList colorSchemeNames() const;

    Q_INVOKABLE void setTerminalFontFamily(const QString& family);
    Q_INVOKABLE void setTerminalFontSize(int size);
    Q_INVOKABLE void setDefaultShell(const QString& shellPath);
    Q_INVOKABLE void setStartDirectory(const QString& directory);
    Q_INVOKABLE void setConfirmCloseRunningProcesses(bool enabled);

    Q_INVOKABLE void setActiveProfile(const QString& name);
    Q_INVOKABLE QString profileShell(const QString& name) const;
    Q_INVOKABLE QString profileStartDirectory(const QString& name) const;
    Q_INVOKABLE QString profileColorScheme(const QString& name) const;
    Q_INVOKABLE bool profileRemovable(const QString& name) const;
    Q_INVOKABLE bool createProfile(const QString& name);
    Q_INVOKABLE bool removeProfile(const QString& name);
    Q_INVOKABLE void setProfileShell(const QString& name, const QString& shellPath);
    Q_INVOKABLE void setProfileStartDirectory(const QString& name, const QString& directory);
    Q_INVOKABLE void setProfileColorScheme(const QString& name, const QString& colorScheme);
    Q_INVOKABLE void setProfileSettings(const QString& name, const QString& shellPath,
                                        const QString& directory, const QString& colorScheme);

    Q_INVOKABLE void resetDefaults();
    void flush();

signals:
    void terminalFontFamilyChanged();
    void terminalFontSizeChanged();
    void defaultShellChanged();
    void startDirectoryChanged();
    void confirmCloseRunningProcessesChanged();
    void profilesChanged();
    void profileColorSchemeChanged(const QString& profileName, const QString& colorScheme);
    void activeProfileChanged();
    void defaultsReset();

private:
    struct Profile {
        QString shell;
        QString startDirectory;
        QString colorScheme;
    };

    static constexpr int DefaultTerminalFontSize = 14;

    void load();
    void persist(const QString& key, const QVariant& value);
    void queueRemove(const QString& keyPrefix);
    void resetProfilesToDefaults();
    void loadProfiles();
    void persistProfileNames();
    void persistProfile(const QString& name);
    [[nodiscard]] QString validatedProfileName(const QString& name) const;
    [[nodiscard]] QString profileStorageKey(const QString& name) const;
    [[nodiscard]] QString normalizedColorScheme(const QString& colorScheme) const;
    [[nodiscard]] Profile defaultProfileForName(const QString& name) const;

    QSettings m_settings;
    QHash<QString, QVariant> m_pendingWrites;
    QStringList m_pendingRemovals;
    bool m_pendingClear{false};
    QString m_terminalFontFamily{QStringLiteral("monospace")};
    int m_terminalFontSize{DefaultTerminalFontSize};
    QString m_defaultShell;
    QString m_startDirectory;
    bool m_confirmCloseRunningProcesses{true};
    QStringList m_profileNames;
    QHash<QString, Profile> m_profiles;
    QString m_activeProfile{QStringLiteral("Default")};
};
