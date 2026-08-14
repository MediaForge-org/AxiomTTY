#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QTimer>

class AppSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString terminalFontFamily READ terminalFontFamily WRITE setTerminalFontFamily NOTIFY terminalFontFamilyChanged)
    Q_PROPERTY(int terminalFontSize READ terminalFontSize WRITE setTerminalFontSize NOTIFY terminalFontSizeChanged)
    Q_PROPERTY(QString defaultShell READ defaultShell WRITE setDefaultShell NOTIFY defaultShellChanged)
    Q_PROPERTY(QString startDirectory READ startDirectory WRITE setStartDirectory NOTIFY startDirectoryChanged)
    Q_PROPERTY(bool inheritWorkingDirectory READ inheritWorkingDirectory WRITE setInheritWorkingDirectory NOTIFY inheritWorkingDirectoryChanged)
    Q_PROPERTY(bool confirmCloseRunningProcesses READ confirmCloseRunningProcesses WRITE setConfirmCloseRunningProcesses NOTIFY confirmCloseRunningProcessesChanged)

public:
    explicit AppSettings(QObject* parent = nullptr);

    [[nodiscard]] QString terminalFontFamily() const;
    [[nodiscard]] int terminalFontSize() const noexcept;
    [[nodiscard]] QString defaultShell() const;
    [[nodiscard]] QString startDirectory() const;
    [[nodiscard]] bool inheritWorkingDirectory() const noexcept;
    [[nodiscard]] bool confirmCloseRunningProcesses() const noexcept;

    Q_INVOKABLE void setTerminalFontFamily(const QString& family);
    Q_INVOKABLE void setTerminalFontSize(int size);
    Q_INVOKABLE void setDefaultShell(const QString& shellPath);
    Q_INVOKABLE void setStartDirectory(const QString& directory);
    Q_INVOKABLE void setInheritWorkingDirectory(bool enabled);
    Q_INVOKABLE void setConfirmCloseRunningProcesses(bool enabled);
    Q_INVOKABLE void resetDefaults();
    void flush();

signals:
    void terminalFontFamilyChanged();
    void terminalFontSizeChanged();
    void defaultShellChanged();
    void startDirectoryChanged();
    void inheritWorkingDirectoryChanged();
    void confirmCloseRunningProcessesChanged();
    void defaultsReset();

private:
    static constexpr int DefaultTerminalFontSize = 14;

    void load();
    void persist(const QString& key, const QVariant& value);
    void scheduleSync();

    QSettings m_settings;
    QTimer m_syncTimer;
    QString m_terminalFontFamily{QStringLiteral("monospace")};
    int m_terminalFontSize{DefaultTerminalFontSize};
    QString m_defaultShell;
    QString m_startDirectory;
    bool m_inheritWorkingDirectory{true};
    bool m_confirmCloseRunningProcesses{true};
};
