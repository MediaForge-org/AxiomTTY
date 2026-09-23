#include "SplitNode.h"

#include "TerminalSession.h"

#include <algorithm>

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

int SplitNode::horizontalSpan() const noexcept
{
    return paneSpan(Qt::Horizontal);
}

int SplitNode::verticalSpan() const noexcept
{
    return paneSpan(Qt::Vertical);
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


TerminalSession* SplitNode::neighborSession(TerminalSession* target, PaneDirection direction) const
{
    if (target == nullptr || isLeaf()) {
        return nullptr;
    }

    QVector<PathStep> path;
    if (!collectPath(target, path)) {
        return nullptr;
    }

    for (qsizetype index = path.size(); index > 0; --index) {
        const PathStep& step = path.at(index - 1);
        if (step.node == nullptr) {
            continue;
        }

        const bool horizontal = step.node->m_orientation == Qt::Horizontal;
        const bool vertical = step.node->m_orientation == Qt::Vertical;
        const SplitNode* sibling = nullptr;

        if (direction == PaneDirection::Left && horizontal && step.fromSecond) {
            sibling = step.node->m_first;
        } else if (direction == PaneDirection::Right && horizontal && !step.fromSecond) {
            sibling = step.node->m_second;
        } else if (direction == PaneDirection::Up && vertical && step.fromSecond) {
            sibling = step.node->m_first;
        } else if (direction == PaneDirection::Down && vertical && !step.fromSecond) {
            sibling = step.node->m_second;
        }

        if (sibling != nullptr) {
            return sibling->edgeLeaf(direction);
        }
    }

    return nullptr;
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
        // Prefer the leaf directly across the removed split boundary. This
        // makes focus after closing a pane feel spatially predictable.
        fallbackSession = m_second != nullptr
            ? m_second->edgeLeaf(PaneDirection::Right)
            : nullptr;
        promoteChild(m_second, m_first);
        return true;
    }

    if (m_second != nullptr && m_second->isLeaf() && m_second->session() == target) {
        fallbackSession = m_first != nullptr
            ? m_first->edgeLeaf(PaneDirection::Left)
            : nullptr;
        promoteChild(m_first, m_second);
        return true;
    }

    if (m_first != nullptr && m_first->removeSession(target, fallbackSession)) {
        return true;
    }
    return m_second != nullptr && m_second->removeSession(target, fallbackSession);
}


bool SplitNode::collectPath(TerminalSession* target, QVector<PathStep>& path) const
{
    if (target == nullptr) {
        return false;
    }
    if (isLeaf()) {
        return m_session == target;
    }

    if (m_first != nullptr) {
        path.push_back(PathStep{this, false});
        if (m_first->collectPath(target, path)) {
            return true;
        }
        path.removeLast();
    }

    if (m_second != nullptr) {
        path.push_back(PathStep{this, true});
        if (m_second->collectPath(target, path)) {
            return true;
        }
        path.removeLast();
    }

    return false;
}

TerminalSession* SplitNode::edgeLeaf(PaneDirection direction) const
{
    if (isLeaf()) {
        return m_session;
    }

    const bool preferSecond = direction == PaneDirection::Left || direction == PaneDirection::Up;
    const SplitNode* preferred = preferSecond ? m_second : m_first;
    const SplitNode* fallback = preferSecond ? m_first : m_second;

    if (preferred != nullptr) {
        if (TerminalSession* session = preferred->edgeLeaf(direction); session != nullptr) {
            return session;
        }
    }
    return fallback != nullptr ? fallback->edgeLeaf(direction) : nullptr;
}

int SplitNode::paneSpan(Qt::Orientation orientation) const noexcept
{
    if (isLeaf()) {
        return 1;
    }

    const int firstSpan = m_first != nullptr ? m_first->paneSpan(orientation) : 0;
    const int secondSpan = m_second != nullptr ? m_second->paneSpan(orientation) : 0;

    // Consecutive splits in the same direction form equal-sized slots. A split
    // in the perpendicular direction shares the same slot on this axis, so its
    // span is the larger child span instead of the sum. This lets an arbitrarily
    // nested binary split tree render as an evenly distributed grid.
    if (m_orientation == orientation) {
        return std::max(1, firstSpan + secondSpan);
    }
    return std::max(1, std::max(firstSpan, secondSpan));
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
