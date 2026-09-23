#include "SessionManager.h"

#include "app/AppSettings.h"
#include "SplitNode.h"
#include "TerminalSession.h"

#include <QDir>
#include <QFileInfo>
#include <QVariant>

#include <algorithm>

SessionManager::SessionManager(AppSettings* settings, QObject* parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
{
    m_cwdRefreshTimer.setInterval(650);
    m_cwdRefreshTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_cwdRefreshTimer, &QTimer::timeout, this, &SessionManager::refreshWorkingDirectories);
    m_cwdRefreshTimer.start();

    if (m_settings != nullptr) {
        connect(m_settings, &AppSettings::profilesChanged, this, &SessionManager::refreshSessionProfiles);
        connect(m_settings, &AppSettings::profileColorSchemeChanged,
                this, &SessionManager::applyProfileColorScheme);
    }

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
    case ProfileRole:
        return tab.profileName;
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
        {ProfileRole, "profileName"},
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

int SessionManager::activePaneIndex() const noexcept
{
    const TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->activeSession == nullptr) {
        return 0;
    }

    const QVector<TerminalSession*> leaves = tab->root->leafSessions();
    const qsizetype index = leaves.indexOf(tab->activeSession);
    return index >= 0 ? static_cast<int>(index) + 1 : 0;
}

bool SessionManager::canSplitActivePane() const noexcept
{
    return activePaneCount() > 0 && activePaneCount() < MaxPanesPerTab;
}

int SessionManager::maxPanesPerTab() const noexcept
{
    return MaxPanesPerTab;
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
    newTabWithProfile(configuredProfileName());
}

void SessionManager::newTabWithProfile(const QString& profileName)
{
    const QString resolvedProfile = configuredProfileName(profileName);
    // Fresh tabs are profile-defined sessions. Preserving the current work
    // context is explicit via Duplicate Tab or split/duplicate pane actions.
    const QString directory = configuredStartDirectory(resolvedProfile);
    insertTab(rowCount(), configuredDefaultShell(resolvedProfile), directory, {}, resolvedProfile);
}

void SessionManager::duplicateTab(int index)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    TabState& sourceTab = m_tabs[index];
    TerminalSession* source = sourceTab.activeSession;
    if (source == nullptr) {
        return;
    }

    source->refreshWorkingDirectory();
    const QString workingDirectory = source->workingDirectory().isEmpty()
        ? QDir::homePath()
        : source->workingDirectory();
    const QString shellPath = source->shell();
    const QString customTitle = sourceTab.customTitle;

    insertTab(index + 1, shellPath, workingDirectory, customTitle, sourceTab.profileName);
}

void SessionManager::renameTab(int index, const QString& title)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    const QString normalized = title.trimmed();
    if (m_tabs[index].customTitle == normalized) {
        return;
    }

    m_tabs[index].customTitle = normalized;
    const QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {TitleRole});
}

void SessionManager::resetTabTitle(int index)
{
    renameTab(index, {});
}

void SessionManager::requestCloseTab(int index)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    if (!tabHasBusyProcesses(index) || (m_settings != nullptr && !m_settings->confirmCloseRunningProcesses())) {
        closeTab(index);
        return;
    }

    clearPendingClose();
    m_pendingCloseKind = PendingCloseKind::Tab;
    m_pendingCloseTabIndex = index;
    emit closeConfirmationRequested(
        QStringLiteral("One or more programs are still running in this tab. Close it anyway?"));
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
        emit activePaneIndexChanged();
        emit applicationCloseApproved();
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
        emit activePaneIndexChanged();
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

void SessionManager::duplicateActivePaneRight()
{
    splitActive(Qt::Horizontal, true);
}

void SessionManager::duplicateActivePaneDown()
{
    splitActive(Qt::Vertical, true);
}

void SessionManager::requestCloseActivePane()
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->activeSession == nullptr) {
        return;
    }

    if (tab->sessions.size() <= 1) {
        requestCloseTab(m_currentIndex);
        return;
    }

    if (!activePaneHasBusyProcess() || (m_settings != nullptr && !m_settings->confirmCloseRunningProcesses())) {
        closeActivePane();
        return;
    }

    clearPendingClose();
    m_pendingCloseKind = PendingCloseKind::Pane;
    m_pendingCloseTabIndex = m_currentIndex;
    m_pendingCloseSession = tab->activeSession;
    emit closeConfirmationRequested(
        QStringLiteral("A program is still running in this pane. Close the pane anyway?"));
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

    emitCurrentTabStateChanged({SessionRole, RootRole, TitleRole, RunningRole, WorkingDirectoryRole, ShellRole, PaneCountRole, ProfileRole});
    emit activeSessionChanged();
    emit activeRootChanged();
    emit activePaneCountChanged();
    emit activePaneIndexChanged();
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
    const qsizetype current = leaves.indexOf(tab->activeSession);
    const qsizetype next = current < 0 ? 0 : (current + 1) % leaves.size();
    setActivePane(*tab, leaves.at(next));
}

void SessionManager::previousPane()
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->sessions.size() <= 1) {
        return;
    }

    const QVector<TerminalSession*> leaves = tab->root->leafSessions();
    const qsizetype current = leaves.indexOf(tab->activeSession);
    const qsizetype previous = current < 0 ? 0 : (current - 1 + leaves.size()) % leaves.size();
    setActivePane(*tab, leaves.at(previous));
}

void SessionManager::focusPaneLeft()
{
    focusPane(SplitNode::PaneDirection::Left);
}

void SessionManager::focusPaneRight()
{
    focusPane(SplitNode::PaneDirection::Right);
}

void SessionManager::focusPaneUp()
{
    focusPane(SplitNode::PaneDirection::Up);
}

void SessionManager::focusPaneDown()
{
    focusPane(SplitNode::PaneDirection::Down);
}

void SessionManager::requestCloseApplication()
{
    if (!anyBusyProcesses() || (m_settings != nullptr && !m_settings->confirmCloseRunningProcesses())) {
        emit applicationCloseApproved();
        return;
    }

    clearPendingClose();
    m_pendingCloseKind = PendingCloseKind::Application;
    emit closeConfirmationRequested(
        QStringLiteral("One or more programs are still running in AxiomTTY. Close the application anyway?"));
}

void SessionManager::confirmPendingClose()
{
    const PendingCloseKind kind = m_pendingCloseKind;
    const int tabIndex = m_pendingCloseTabIndex;
    TerminalSession* session = m_pendingCloseSession;
    clearPendingClose();

    if (kind == PendingCloseKind::Tab) {
        closeTab(tabIndex);
        return;
    }

    if (kind == PendingCloseKind::Application) {
        emit applicationCloseApproved();
        return;
    }

    if (kind != PendingCloseKind::Pane || tabIndex != m_currentIndex) {
        return;
    }

    TabState* tab = currentTab();
    if (tab == nullptr || session == nullptr || tab->activeSession != session) {
        return;
    }
    closeActivePane();
}

void SessionManager::cancelPendingClose()
{
    clearPendingClose();
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
    emit activePaneIndexChanged();
}

TerminalSession* SessionManager::createSession(const QString& workingDirectory, const QString& profileName)
{
    auto* session = new TerminalSession(this);
    const QString resolvedProfile = configuredProfileName(profileName);
    session->setInitialWorkingDirectory(workingDirectory);
    session->setProfile(resolvedProfile, configuredColorScheme(resolvedProfile));
    connectSession(session);
    return session;
}

void SessionManager::insertTab(int index, const QString& shellPath, const QString& workingDirectory, const QString& customTitle, const QString& profileName)
{
    const int insertIndex = std::clamp(index, 0, rowCount());
    const QString directory = workingDirectory.isEmpty() ? QDir::homePath() : workingDirectory;

    const QString resolvedProfile = configuredProfileName(profileName);
    TerminalSession* session = createSession(directory, resolvedProfile);
    auto* root = new SplitNode(session, this);

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    TabState tab;
    tab.root = root;
    tab.activeSession = session;
    tab.sessions.push_back(session);
    tab.customTitle = customTitle.trimmed();
    tab.profileName = resolvedProfile;
    m_tabs.insert(insertIndex, tab);
    endInsertRows();
    emit countChanged();

    setCurrentIndex(insertIndex);

    if (!shellPath.isEmpty()) {
        session->startShellInDirectory(shellPath, directory);
    } else {
        session->startDefaultShellInDirectory(directory);
    }
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
    if (!tab.customTitle.isEmpty()) {
        return tab.customTitle;
    }

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

QString SessionManager::configuredProfileName(const QString& profileName) const
{
    if (m_settings == nullptr) {
        return QStringLiteral("Default");
    }
    const QString requested = profileName.trimmed().isEmpty() ? m_settings->activeProfile() : profileName.trimmed();
    return m_settings->profileNames().contains(requested) ? requested : QStringLiteral("Default");
}

QString SessionManager::configuredStartDirectory(const QString& profileName) const
{
    QString directory = m_settings != nullptr
        ? m_settings->profileStartDirectory(configuredProfileName(profileName)).trimmed()
        : QDir::homePath();
    if (directory.isEmpty() || directory == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (directory.startsWith(QStringLiteral("~/"))) {
        directory = QDir::homePath() + directory.mid(1);
    }
    return QFileInfo(directory).isDir() ? QDir(directory).absolutePath() : QDir::homePath();
}

QString SessionManager::configuredDefaultShell(const QString& profileName) const
{
    if (m_settings == nullptr) {
        return {};
    }
    const QString shell = m_settings->profileShell(configuredProfileName(profileName)).trimmed();
    if (shell.isEmpty()) {
        return {};
    }
    const QFileInfo info(shell);
    return info.isFile() && info.isExecutable() ? info.absoluteFilePath() : QString{};
}

QString SessionManager::configuredColorScheme(const QString& profileName) const
{
    return m_settings != nullptr
        ? m_settings->profileColorScheme(configuredProfileName(profileName))
        : QStringLiteral("Axiom Dark");
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

void SessionManager::refreshSessionProfiles()
{
    for (TabState& tab : m_tabs) {
        const QString resolvedProfile = configuredProfileName(tab.profileName);
        tab.profileName = resolvedProfile;
        const QString scheme = configuredColorScheme(resolvedProfile);
        for (TerminalSession* session : tab.sessions) {
            if (session != nullptr) {
                session->setProfile(resolvedProfile, scheme);
            }
        }
    }
    if (rowCount() > 0) {
        emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0), {ProfileRole});
    }
    emit activeSessionChanged();
}

void SessionManager::applyProfileColorScheme(const QString& profileName, const QString& colorScheme)
{
    bool activeChanged = false;
    for (int tabIndex = 0; tabIndex < rowCount(); ++tabIndex) {
        TabState& tab = m_tabs[tabIndex];
        if (tab.profileName != profileName) {
            continue;
        }
        for (TerminalSession* session : tab.sessions) {
            if (session != nullptr) {
                session->setProfile(profileName, colorScheme);
            }
        }
        const QModelIndex modelIndex = createIndex(tabIndex, 0);
        emit dataChanged(modelIndex, modelIndex, {ProfileRole});
        if (tabIndex == m_currentIndex) {
            activeChanged = true;
        }
    }
    if (activeChanged) {
        emit activeSessionChanged();
    }
}

void SessionManager::splitActive(Qt::Orientation orientation, bool duplicateShell)
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->activeSession == nullptr) {
        return;
    }

    // A huge recursive split tree quickly becomes unusable and can create
    // panes smaller than real TUI programs can handle. Keep a deliberate,
    // predictable per-tab ceiling. The UI disables split controls at the same
    // limit, while this guard also covers keyboard shortcuts.
    if (tab->sessions.size() >= MaxPanesPerTab) {
        return;
    }

    TerminalSession* current = tab->activeSession;
    current->refreshWorkingDirectory();
    const QString workingDirectory = current->workingDirectory().isEmpty()
        ? QDir::homePath()
        : current->workingDirectory();

    TerminalSession* session = createSession(workingDirectory, tab->profileName);
    if (!tab->root->splitSession(current, orientation, session)) {
        session->deleteLater();
        return;
    }

    tab->sessions.push_back(session);
    tab->activeSession = session;

    emitCurrentTabStateChanged({SessionRole, RootRole, TitleRole, RunningRole, WorkingDirectoryRole, ShellRole, PaneCountRole, ProfileRole});
    emit activeSessionChanged();
    emit activeRootChanged();
    emit activePaneCountChanged();
    emit activePaneIndexChanged();

    if (duplicateShell && !current->shell().isEmpty()) {
        session->startShellInDirectory(current->shell(), workingDirectory);
    } else {
        const QString shell = configuredDefaultShell(tab->profileName);
        if (!shell.isEmpty()) {
            session->startShellInDirectory(shell, workingDirectory);
        } else {
            session->startDefaultShellInDirectory(workingDirectory);
        }
    }
}

void SessionManager::focusPane(SplitNode::PaneDirection direction)
{
    TabState* tab = currentTab();
    if (tab == nullptr || tab->root == nullptr || tab->activeSession == nullptr) {
        return;
    }

    TerminalSession* neighbor = tab->root->neighborSession(tab->activeSession, direction);
    if (neighbor != nullptr) {
        setActivePane(*tab, neighbor);
    }
}

bool SessionManager::tabHasBusyProcesses(int index) const
{
    if (index < 0 || index >= rowCount()) {
        return false;
    }

    const TabState& tab = m_tabs.at(index);
    return std::any_of(tab.sessions.cbegin(), tab.sessions.cend(), [](const TerminalSession* session) {
        return session != nullptr && session->hasChildProcesses();
    });
}

bool SessionManager::activePaneHasBusyProcess() const
{
    const TabState* tab = currentTab();
    return tab != nullptr
        && tab->activeSession != nullptr
        && tab->activeSession->hasChildProcesses();
}

bool SessionManager::anyBusyProcesses() const
{
    for (const TabState& tab : m_tabs) {
        const bool busy = std::any_of(tab.sessions.cbegin(), tab.sessions.cend(), [](const TerminalSession* session) {
            return session != nullptr && session->hasChildProcesses();
        });
        if (busy) {
            return true;
        }
    }
    return false;
}

void SessionManager::clearPendingClose()
{
    m_pendingCloseKind = PendingCloseKind::None;
    m_pendingCloseTabIndex = -1;
    m_pendingCloseSession = nullptr;
}

void SessionManager::setActivePane(TabState& tab, TerminalSession* session)
{
    if (session == nullptr || tab.activeSession == session || !tab.sessions.contains(session)) {
        return;
    }

    tab.activeSession = session;
    emitCurrentTabStateChanged({SessionRole, TitleRole, WorkingDirectoryRole, ShellRole});
    emit activeSessionChanged();
    emit activePaneIndexChanged();
}

void SessionManager::emitCurrentTabStateChanged(const QVector<int>& roles)
{
    if (m_currentIndex < 0 || m_currentIndex >= rowCount()) {
        return;
    }
    const QModelIndex modelIndex = createIndex(m_currentIndex, 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}
