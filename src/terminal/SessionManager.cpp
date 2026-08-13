#include "SessionManager.h"

#include "SplitNode.h"
#include "TerminalSession.h"

#include <QDir>
#include <QFileInfo>
#include <QVariant>

#include <algorithm>

SessionManager::SessionManager(QObject* parent)
    : QAbstractListModel(parent)
{
    m_cwdRefreshTimer.setInterval(650);
    m_cwdRefreshTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_cwdRefreshTimer, &QTimer::timeout, this, &SessionManager::refreshWorkingDirectories);
    m_cwdRefreshTimer.start();

    newTab();
}

int SessionManager::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_tabs.size());
}

QVariant SessionManager::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const TabState& tab = m_tabs.at(index.row());
    TerminalSession* session = tab.activeSession;

    switch (role) {
    case SessionRole:
        return QVariant::fromValue(static_cast<QObject*>(session));
    case RootRole:
        return QVariant::fromValue(static_cast<QObject*>(tab.root));
    case TitleRole:
        return displayTitle(tab);
    case RunningRole:
        return anyRunning(tab);
    case WorkingDirectoryRole:
        return session != nullptr ? session->workingDirectory() : QString{};
    case ShellRole:
        return session != nullptr ? session->shell() : QString{};
    case PaneCountRole:
        return static_cast<int>(tab.sessions.size());
    default:
        return {};
    }
}

QHash<int, QByteArray> SessionManager::roleNames() const
{
    return {
        {SessionRole, "sessionObject"},
        {RootRole, "rootNode"},
        {TitleRole, "displayTitle"},
        {RunningRole, "isRunning"},
        {WorkingDirectoryRole, "workingDirectory"},
        {ShellRole, "shellPath"},
        {PaneCountRole, "paneCount"},
    };
}

QObject* SessionManager::activeSession() const
{
    const TabState* tab = currentTab();
    return tab != nullptr ? static_cast<QObject*>(tab->activeSession.data()) : nullptr;
}

QObject* SessionManager::activeRoot() const
{
    const TabState* tab = currentTab();
    return tab != nullptr ? static_cast<QObject*>(tab->root) : nullptr;
}

int SessionManager::activePaneCount() const noexcept
{
    const TabState* tab = currentTab();
    return tab != nullptr ? static_cast<int>(tab->sessions.size()) : 0;
}

int SessionManager::currentIndex() const noexcept
{
    return m_currentIndex;
}

int SessionManager::count() const noexcept
{
    return static_cast<int>(m_tabs.size());
}

void SessionManager::newTab()
{
    const QString workingDirectory = inheritedWorkingDirectory();
    const int insertIndex = rowCount();

    TerminalSession* session = createSession(workingDirectory);
    auto* root = new SplitNode(session, this);

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    TabState tab;
    tab.root = root;
    tab.activeSession = session;
    tab.sessions.push_back(session);
    m_tabs.push_back(tab);
    endInsertRows();
    emit countChanged();

    setCurrentIndex(insertIndex);
    session->startDefaultShellInDirectory(workingDirectory);
}

void SessionManager::closeTab(int index)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    const bool closingCurrent = index == m_currentIndex;
    const int oldCurrent = m_currentIndex;

    beginRemoveRows(QModelIndex(), index, index);
    TabState tab = m_tabs.takeAt(index);
    endRemoveRows();
    emit countChanged();

    if (tab.root != nullptr) {
        tab.root->deleteLater();
    }
    for (TerminalSession* session : tab.sessions) {
        if (session != nullptr) {
            session->deleteLater();
        }
    }

    if (m_tabs.isEmpty()) {
        m_currentIndex = -1;
        emit currentIndexChanged();
        emit activeSessionChanged();
        emit activeRootChanged();
        emit activePaneCountChanged();
        newTab();
        return;
    }

    int nextIndex = oldCurrent;
    if (closingCurrent) {
        nextIndex = std::min(index, rowCount() - 1);
    } else if (index < oldCurrent) {
        nextIndex = oldCurrent - 1;
    }

    if (m_currentIndex != nextIndex) {
        m_currentIndex = nextIndex;
        emit currentIndexChanged();
    }

    if (closingCurrent || index < oldCurrent) {
        emit activeSessionChanged();
        emit activeRootChanged();
        emit activePaneCountChanged();
    }
}

void SessionManager::activateTab(int index)
{
    setCurrentIndex(index);
}

void SessionManager::nextTab()
{
    const int tabCount = rowCount();
    if (tabCount <= 1) {
        return;
    }
    setCurrentIndex((m_currentIndex + 1) % tabCount);
}

void SessionManager::previousTab()
{
    const int tabCount = rowCount();
    if (tabCount <= 1) {
        return;
    }
    setCurrentIndex((m_currentIndex - 1 + tabCount) % tabCount);
}

void SessionManager::activateTabNumber(int number)
{
    if (number <= 0) {
        return;
    }
    const int index = number - 1;
    if (index < rowCount()) {
        setCurrentIndex(index);
    }
}

void SessionManager::splitRight()
{
    splitActive(Qt::Horizontal);
}

void SessionManager::splitDown()
{
    splitActive(Qt::Vertical);
}

void SessionManager::closeActivePane()
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->activeSession == nullptr) {
        return;
    }

    if (tab->sessions.size() <= 1) {
        closeTab(m_currentIndex);
        return;
    }

    TerminalSession* closingSession = tab->activeSession;
    TerminalSession* fallback = nullptr;
    if (tab->root == nullptr || !tab->root->removeSession(closingSession, fallback)) {
        return;
    }

    tab->sessions.removeOne(closingSession);
    if (fallback == nullptr && tab->root != nullptr) {
        fallback = tab->root->firstLeafSession();
    }
    tab->activeSession = fallback;
    closingSession->deleteLater();

    emitCurrentTabStateChanged({SessionRole, RootRole, TitleRole, RunningRole, WorkingDirectoryRole, ShellRole, PaneCountRole});
    emit activeSessionChanged();
    emit activeRootChanged();
    emit activePaneCountChanged();
}

void SessionManager::activatePane(QObject* sessionObject)
{
    auto* session = qobject_cast<TerminalSession*>(sessionObject);
    TabState* tab = currentTab();
    if (session == nullptr || tab == nullptr || !tab->sessions.contains(session)) {
        return;
    }
    setActivePane(*tab, session);
}

void SessionManager::nextPane()
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->sessions.size() <= 1) {
        return;
    }

    const QVector<TerminalSession*> leaves = tab->root->leafSessions();
    const int current = leaves.indexOf(tab->activeSession);
    const int next = current < 0 ? 0 : (current + 1) % leaves.size();
    setActivePane(*tab, leaves.at(next));
}

void SessionManager::previousPane()
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->sessions.size() <= 1) {
        return;
    }

    const QVector<TerminalSession*> leaves = tab->root->leafSessions();
    const int current = leaves.indexOf(tab->activeSession);
    const int previous = current < 0 ? 0 : (current - 1 + leaves.size()) % leaves.size();
    setActivePane(*tab, leaves.at(previous));
}

void SessionManager::setCurrentIndex(int index)
{
    if (index < 0 || index >= rowCount() || index == m_currentIndex) {
        return;
    }

    m_currentIndex = index;
    emit currentIndexChanged();
    emit activeSessionChanged();
    emit activeRootChanged();
    emit activePaneCountChanged();
}

TerminalSession* SessionManager::createSession(const QString& workingDirectory)
{
    auto* session = new TerminalSession(this);
    session->setInitialWorkingDirectory(workingDirectory);
    connectSession(session);
    return session;
}

SessionManager::TabState* SessionManager::currentTab()
{
    if (m_currentIndex < 0 || m_currentIndex >= rowCount()) {
        return nullptr;
    }
    return &m_tabs[m_currentIndex];
}

const SessionManager::TabState* SessionManager::currentTab() const
{
    if (m_currentIndex < 0 || m_currentIndex >= rowCount()) {
        return nullptr;
    }
    return &m_tabs.at(m_currentIndex);
}

TerminalSession* SessionManager::sessionAtTab(int index) const
{
    if (index < 0 || index >= rowCount()) {
        return nullptr;
    }
    return m_tabs.at(index).activeSession;
}

QString SessionManager::displayTitle(const TabState& tab) const
{
    const TerminalSession* session = tab.activeSession;
    if (session == nullptr) {
        return QStringLiteral("Shell");
    }

    const QString shellName = QFileInfo(session->shell()).fileName();
    const QString terminalTitle = session->title().trimmed();
    if (!terminalTitle.isEmpty()
        && terminalTitle != QStringLiteral("Shell")
        && terminalTitle != shellName) {
        return terminalTitle;
    }

    const QString directory = session->workingDirectory();
    if (directory == QDir::homePath()) {
        return QStringLiteral("~");
    }

    const QString directoryName = QFileInfo(directory).fileName();
    if (!directoryName.isEmpty()) {
        return directoryName;
    }

    if (!shellName.isEmpty()) {
        return shellName;
    }
    return QStringLiteral("Shell");
}

QString SessionManager::inheritedWorkingDirectory() const
{
    TerminalSession* current = sessionAtTab(m_currentIndex);
    if (current == nullptr) {
        return QDir::homePath();
    }

    current->refreshWorkingDirectory();
    const QString directory = current->workingDirectory();
    return directory.isEmpty() ? QDir::homePath() : directory;
}

int SessionManager::tabIndexForSession(const TerminalSession* session) const
{
    if (session == nullptr) {
        return -1;
    }
    for (int index = 0; index < rowCount(); ++index) {
        if (m_tabs.at(index).sessions.contains(const_cast<TerminalSession*>(session))) {
            return index;
        }
    }
    return -1;
}

bool SessionManager::anyRunning(const TabState& tab) const
{
    return std::any_of(tab.sessions.cbegin(), tab.sessions.cend(), [](const TerminalSession* session) {
        return session != nullptr && session->running();
    });
}

void SessionManager::connectSession(TerminalSession* session)
{
    connect(session, &TerminalSession::titleChanged, this, [this, session] {
        notifySessionChanged(session, {TitleRole});
    });
    connect(session, &TerminalSession::runningChanged, this, [this, session] {
        notifySessionChanged(session, {RunningRole, TitleRole});
    });
    connect(session, &TerminalSession::workingDirectoryChanged, this, [this, session] {
        notifySessionChanged(session, {WorkingDirectoryRole, TitleRole});
    });
    connect(session, &TerminalSession::shellChanged, this, [this, session] {
        notifySessionChanged(session, {ShellRole, TitleRole});
    });
}

void SessionManager::notifySessionChanged(TerminalSession* session, const QVector<int>& roles)
{
    const int index = tabIndexForSession(session);
    if (index < 0) {
        return;
    }

    const QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, roles);

    if (index == m_currentIndex && currentTab() != nullptr && currentTab()->activeSession == session) {
        emit activeSessionChanged();
    }
}

void SessionManager::refreshWorkingDirectories()
{
    for (TabState& tab : m_tabs) {
        for (TerminalSession* session : tab.sessions) {
            if (session != nullptr) {
                session->refreshWorkingDirectory();
            }
        }
    }
}

void SessionManager::splitActive(Qt::Orientation orientation)
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->activeSession == nullptr) {
        return;
    }

    TerminalSession* current = tab->activeSession;
    current->refreshWorkingDirectory();
    const QString workingDirectory = current->workingDirectory().isEmpty()
        ? QDir::homePath()
        : current->workingDirectory();

    TerminalSession* session = createSession(workingDirectory);
    if (!tab->root->splitSession(current, orientation, session)) {
        session->deleteLater();
        return;
    }

    tab->sessions.push_back(session);
    tab->activeSession = session;

    emitCurrentTabStateChanged({SessionRole, RootRole, TitleRole, RunningRole, WorkingDirectoryRole, ShellRole, PaneCountRole});
    emit activeSessionChanged();
    emit activeRootChanged();
    emit activePaneCountChanged();

    session->startDefaultShellInDirectory(workingDirectory);
}

void SessionManager::setActivePane(TabState& tab, TerminalSession* session)
{
    if (session == nullptr || tab.activeSession == session || !tab.sessions.contains(session)) {
        return;
    }

    tab.activeSession = session;
    emitCurrentTabStateChanged({SessionRole, TitleRole, WorkingDirectoryRole, ShellRole});
    emit activeSessionChanged();
}

void SessionManager::emitCurrentTabStateChanged(const QVector<int>& roles)
{
    if (m_currentIndex < 0 || m_currentIndex >= rowCount()) {
        return;
    }
    const QModelIndex modelIndex = createIndex(m_currentIndex, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}
