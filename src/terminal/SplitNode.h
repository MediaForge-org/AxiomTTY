#pragma once

#include <QObject>
#include <QPointer>
#include <QVector>

class TerminalSession;

class SplitNode final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool leaf READ isLeaf NOTIFY structureChanged)
    Q_PROPERTY(int orientation READ orientationValue NOTIFY structureChanged)
    Q_PROPERTY(QObject* sessionObject READ sessionObject NOTIFY structureChanged)
    Q_PROPERTY(QObject* firstNode READ firstNodeObject NOTIFY structureChanged)
    Q_PROPERTY(QObject* secondNode READ secondNodeObject NOTIFY structureChanged)

public:
    explicit SplitNode(TerminalSession* session, QObject* parent = nullptr);

    [[nodiscard]] bool isLeaf() const noexcept;
    [[nodiscard]] int orientationValue() const noexcept;
    [[nodiscard]] QObject* sessionObject() const;
    [[nodiscard]] QObject* firstNodeObject() const;
    [[nodiscard]] QObject* secondNodeObject() const;

    [[nodiscard]] TerminalSession* session() const noexcept;
    [[nodiscard]] TerminalSession* firstLeafSession() const;
    [[nodiscard]] QVector<TerminalSession*> leafSessions() const;
    [[nodiscard]] bool containsSession(const TerminalSession* session) const;

    bool splitSession(TerminalSession* target, Qt::Orientation orientation, TerminalSession* newSession);
    bool removeSession(TerminalSession* target, TerminalSession*& fallbackSession);

signals:
    void structureChanged();

private:
    void collectLeafSessions(QVector<TerminalSession*>& sessions) const;
    void promoteChild(SplitNode* keep, SplitNode* remove);

    QPointer<TerminalSession> m_session;
    Qt::Orientation m_orientation{Qt::Horizontal};
    SplitNode* m_first{nullptr};
    SplitNode* m_second{nullptr};
};
