#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QPointer>
#include <QTimer>
#include <QVector>

class TerminalSession;

class SessionManager final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QObject* activeSession READ activeSession NOTIFY activeSessionChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        SessionRole = Qt::UserRole + 1,
        TitleRole,
        RunningRole,
        WorkingDirectoryRole,
        ShellRole,
    };
    Q_ENUM(Role)

    explicit SessionManager(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QObject* activeSession() const;
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] int count() const noexcept;

    Q_INVOKABLE void newTab();
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE void nextTab();
    Q_INVOKABLE void previousTab();
    Q_INVOKABLE void activateTabNumber(int number);

public slots:
    void setCurrentIndex(int index);

signals:
    void activeSessionChanged();
    void currentIndexChanged();
    void countChanged();

private:
    TerminalSession* createSession(const QString& workingDirectory);
    [[nodiscard]] TerminalSession* sessionAt(int index) const;
    [[nodiscard]] QString displayTitle(const TerminalSession* session) const;
    [[nodiscard]] QString inheritedWorkingDirectory() const;
    void connectSession(TerminalSession* session);
    void notifySessionChanged(TerminalSession* session, const QVector<int>& roles);
    void refreshWorkingDirectories();

    QVector<TerminalSession*> m_sessions;
    int m_currentIndex{-1};
    QTimer m_cwdRefreshTimer;
};
