#include "SessionManager.h"

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
    return static_cast<int>(m_sessions.size());
}

QVariant SessionManager::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    TerminalSession* session = m_sessions.at(index.row());
    switch (role) {
    case SessionRole:
        return QVariant::fromValue(static_cast<QObject*>(session));
    case TitleRole:
        return displayTitle(session);
    case RunningRole:
        return session->running();
    case WorkingDirectoryRole:
        return session->workingDirectory();
    case ShellRole:
        return session->shell();
    default:
        return {};
    }
}

QHash<int, QByteArray> SessionManager::roleNames() const
{
    return {
        {SessionRole, "sessionObject"},
        {TitleRole, "displayTitle"},
        {RunningRole, "isRunning"},
        {WorkingDirectoryRole, "workingDirectory"},
        {ShellRole, "shellPath"},
    };
}

QObject* SessionManager::activeSession() const
{
    return sessionAt(m_currentIndex);
}

int SessionManager::currentIndex() const noexcept
{
    return m_currentIndex;
}

int SessionManager::count() const noexcept
{
    return static_cast<int>(m_sessions.size());
}

void SessionManager::newTab()
{
    const QString workingDirectory = inheritedWorkingDirectory();
    const int insertIndex = rowCount();

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    TerminalSession* session = createSession(workingDirectory);
    m_sessions.push_back(session);
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
    TerminalSession* session = m_sessions.takeAt(index);
    endRemoveRows();
    emit countChanged();

    session->deleteLater();

    if (m_sessions.isEmpty()) {
        m_currentIndex = -1;
        emit currentIndexChanged();
        emit activeSessionChanged();
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

void SessionManager::setCurrentIndex(int index)
{
    if (index < 0 || index >= rowCount() || index == m_currentIndex) {
        return;
    }

    m_currentIndex = index;
    emit currentIndexChanged();
    emit activeSessionChanged();
}

TerminalSession* SessionManager::createSession(const QString& workingDirectory)
{
    auto* session = new TerminalSession(this);
    session->setInitialWorkingDirectory(workingDirectory);
    connectSession(session);
    return session;
}

TerminalSession* SessionManager::sessionAt(int index) const
{
    if (index < 0 || index >= rowCount()) {
        return nullptr;
    }
    return m_sessions.at(index);
}

QString SessionManager::displayTitle(const TerminalSession* session) const
{
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
    TerminalSession* current = sessionAt(m_currentIndex);
    if (current == nullptr) {
        return QDir::homePath();
    }

    current->refreshWorkingDirectory();
    const QString directory = current->workingDirectory();
    return directory.isEmpty() ? QDir::homePath() : directory;
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
    const int index = m_sessions.indexOf(session);
    if (index < 0) {
        return;
    }
    const QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, roles);

    if (index == m_currentIndex) {
        emit activeSessionChanged();
    }
}

void SessionManager::refreshWorkingDirectories()
{
    for (TerminalSession* session : m_sessions) {
        session->refreshWorkingDirectory();
    }
}
