#include "terminal/SplitNode.h"
#include "terminal/TerminalSession.h"

#include <QCoreApplication>

#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    TerminalSession a;
    TerminalSession b;
    TerminalSession c;
    TerminalSession d;

    SplitNode root(&a);
    assert(root.isLeaf());
    assert(root.firstLeafSession() == &a);

    assert(root.splitSession(&a, Qt::Horizontal, &b));
    assert(!root.isLeaf());
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b}));

    assert(root.splitSession(&b, Qt::Vertical, &c));
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &c}));

    assert(root.splitSession(&c, Qt::Horizontal, &d));
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &c, &d}));

    TerminalSession* fallback = nullptr;
    assert(root.removeSession(&c, fallback));
    assert(fallback == &d);
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &d}));

    fallback = nullptr;
    assert(root.removeSession(&b, fallback));
    assert(fallback == &d);
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &d}));

    fallback = nullptr;
    assert(root.removeSession(&d, fallback));
    assert(fallback == &a);
    assert(root.isLeaf());
    assert(root.session() == &a);

    std::cout << "Split tree smoke OK\n";
    return 0;
}
