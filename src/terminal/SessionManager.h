#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

class SplitNode;
class TerminalSession;

class SessionManager final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QObject* activeSession READ activeSession NOTIFY activeSessionChanged)
    Q_PROPERTY(QObject* activeRoot READ activeRoot NOTIFY activeRootChanged)
    Q_PROPERTY(int activePaneCount READ activePaneCount NOTIFY activePaneCountChanged)
    Q_PROPERTY(int activePaneIndex READ activePaneIndex NOTIFY activePaneIndexChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        SessionRole = Qt::UserRole + 1,
        RootRole,
        TitleRole,
        RunningRole,
        WorkingDirectoryRole,
        ShellRole,
        PaneCountRole,
    };
    Q_ENUM(Role)

    explicit SessionManager(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QObject* activeSession() const;
    [[nodiscard]] QObject* activeRoot() const;
    [[nodiscard]] int activePaneCount() const noexcept;
    [[nodiscard]] int activePaneIndex() const noexcept;
    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] int count() const noexcept;

    Q_INVOKABLE void newTab();
    Q_INVOKABLE void duplicateTab(int index);
    Q_INVOKABLE void renameTab(int index, const QString& title);
    Q_INVOKABLE void resetTabTitle(int index);
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void activateTab(int index);
    Q_INVOKABLE void nextTab();
    Q_INVOKABLE void previousTab();
    Q_INVOKABLE void activateTabNumber(int number);

    Q_INVOKABLE void splitRight();
    Q_INVOKABLE void splitDown();
    Q_INVOKABLE void closeActivePane();
    Q_INVOKABLE void activatePane(QObject* sessionObject);
    Q_INVOKABLE void nextPane();
    Q_INVOKABLE void previousPane();

public slots:
    void setCurrentIndex(int index);

signals:
    void activeSessionChanged();
    void activeRootChanged();
    void activePaneCountChanged();
    void activePaneIndexChanged();
    void currentIndexChanged();
    void countChanged();

private:
    struct TabState {
        SplitNode* root{nullptr};
        QPointer<TerminalSession> activeSession;
        QVector<TerminalSession*> sessions;
        QString customTitle;
    };

    TerminalSession* createSession(const QString& workingDirectory);
    void insertTab(int index, const QString& shellPath, const QString& workingDirectory, const QString& customTitle = {});
    [[nodiscard]] TabState* currentTab();
    [[nodiscard]] const TabState* currentTab() const;
    [[nodiscard]] TerminalSession* sessionAtTab(int index) const;
    [[nodiscard]] QString displayTitle(const TabState& tab) const;
    [[nodiscard]] QString inheritedWorkingDirectory() const;
    [[nodiscard]] int tabIndexForSession(const TerminalSession* session) const;
    [[nodiscard]] bool anyRunning(const TabState& tab) const;
    void connectSession(TerminalSession* session);
    void notifySessionChanged(TerminalSession* session, const QVector<int>& roles);
    void refreshWorkingDirectories();
    void splitActive(Qt::Orientation orientation);
    void setActivePane(TabState& tab, TerminalSession* session);
    void emitCurrentTabStateChanged(const QVector<int>& roles = {});

    QVector<TabState> m_tabs;
    int m_currentIndex{-1};
    QTimer m_cwdRefreshTimer;
};
