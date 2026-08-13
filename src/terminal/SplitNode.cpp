#include "SplitNode.h"

#include "TerminalSession.h"

SplitNode::SplitNode(TerminalSession* session, QObject* parent)
    : QObject(parent)
    , m_session(session)
{
}

bool SplitNode::isLeaf() const noexcept
{
    return m_first == nullptr && m_second == nullptr;
}

int SplitNode::orientationValue() const noexcept
{
    return static_cast<int>(m_orientation);
}

QObject* SplitNode::sessionObject() const
{
    return m_session.data();
}

QObject* SplitNode::firstNodeObject() const
{
    return m_first;
}

QObject* SplitNode::secondNodeObject() const
{
    return m_second;
}

TerminalSession* SplitNode::session() const noexcept
{
    return m_session;
}

TerminalSession* SplitNode::firstLeafSession() const
{
    if (isLeaf()) {
        return m_session;
    }
    if (m_first != nullptr) {
        if (TerminalSession* first = m_first->firstLeafSession(); first != nullptr) {
            return first;
        }
    }
    return m_second != nullptr ? m_second->firstLeafSession() : nullptr;
}

QVector<TerminalSession*> SplitNode::leafSessions() const
{
    QVector<TerminalSession*> result;
    collectLeafSessions(result);
    return result;
}

bool SplitNode::containsSession(const TerminalSession* sessionValue) const
{
    if (sessionValue == nullptr) {
        return false;
    }
    if (isLeaf()) {
        return m_session == sessionValue;
    }
    return (m_first != nullptr && m_first->containsSession(sessionValue))
        || (m_second != nullptr && m_second->containsSession(sessionValue));
}

bool SplitNode::splitSession(TerminalSession* target, Qt::Orientation orientationValue, TerminalSession* newSession)
{
    if (target == nullptr || newSession == nullptr) {
        return false;
    }

    if (isLeaf()) {
        if (m_session != target) {
            return false;
        }

        TerminalSession* existing = m_session;
        m_session = nullptr;
        m_orientation = orientationValue;
        m_first = new SplitNode(existing, this);
        m_second = new SplitNode(newSession, this);
        emit structureChanged();
        return true;
    }

    if (m_first != nullptr && m_first->splitSession(target, orientationValue, newSession)) {
        return true;
    }
    return m_second != nullptr && m_second->splitSession(target, orientationValue, newSession);
}

bool SplitNode::removeSession(TerminalSession* target, TerminalSession*& fallbackSession)
{
    if (target == nullptr || isLeaf()) {
        return false;
    }

    if (m_first != nullptr && m_first->isLeaf() && m_first->session() == target) {
        fallbackSession = m_second != nullptr ? m_second->firstLeafSession() : nullptr;
        promoteChild(m_second, m_first);
        return true;
    }

    if (m_second != nullptr && m_second->isLeaf() && m_second->session() == target) {
        fallbackSession = m_first != nullptr ? m_first->firstLeafSession() : nullptr;
        promoteChild(m_first, m_second);
        return true;
    }

    if (m_first != nullptr && m_first->removeSession(target, fallbackSession)) {
        return true;
    }
    return m_second != nullptr && m_second->removeSession(target, fallbackSession);
}

void SplitNode::collectLeafSessions(QVector<TerminalSession*>& sessions) const
{
    if (isLeaf()) {
        if (m_session != nullptr) {
            sessions.push_back(m_session);
        }
        return;
    }

    if (m_first != nullptr) {
        m_first->collectLeafSessions(sessions);
    }
    if (m_second != nullptr) {
        m_second->collectLeafSessions(sessions);
    }
}

void SplitNode::promoteChild(SplitNode* keep, SplitNode* remove)
{
    if (keep == nullptr || remove == nullptr) {
        return;
    }

    TerminalSession* promotedSession = keep->m_session;
    const Qt::Orientation promotedOrientation = keep->m_orientation;
    SplitNode* promotedFirst = keep->m_first;
    SplitNode* promotedSecond = keep->m_second;

    if (promotedFirst != nullptr) {
        promotedFirst->setParent(this);
    }
    if (promotedSecond != nullptr) {
        promotedSecond->setParent(this);
    }

    keep->m_first = nullptr;
    keep->m_second = nullptr;
    keep->m_session = nullptr;

    m_session = promotedSession;
    m_orientation = promotedOrientation;
    m_first = promotedFirst;
    m_second = promotedSecond;

    remove->deleteLater();
    keep->deleteLater();
    emit structureChanged();
}
