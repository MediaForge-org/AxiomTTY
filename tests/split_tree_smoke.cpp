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
    assert(root.neighborSession(&a, SplitNode::PaneDirection::Right) == &b);
    assert(root.neighborSession(&b, SplitNode::PaneDirection::Left) == &a);
    assert(root.neighborSession(&a, SplitNode::PaneDirection::Left) == nullptr);
    assert(root.horizontalSpan() == 2);
    assert(root.verticalSpan() == 1);

    assert(root.splitSession(&b, Qt::Vertical, &c));
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &c}));
    assert(root.neighborSession(&b, SplitNode::PaneDirection::Down) == &c);
    assert(root.neighborSession(&c, SplitNode::PaneDirection::Up) == &b);
    assert(root.neighborSession(&c, SplitNode::PaneDirection::Left) == &a);
    assert(root.horizontalSpan() == 2);
    assert(root.verticalSpan() == 2);

    assert(root.splitSession(&c, Qt::Horizontal, &d));
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &c, &d}));
    assert(root.neighborSession(&c, SplitNode::PaneDirection::Right) == &d);
    assert(root.neighborSession(&d, SplitNode::PaneDirection::Left) == &c);
    assert(root.neighborSession(&d, SplitNode::PaneDirection::Up) == &b);
    assert(root.neighborSession(&a, SplitNode::PaneDirection::Right) == &b);
    assert(root.horizontalSpan() == 3);
    assert(root.verticalSpan() == 2);

    TerminalSession* fallback = nullptr;
    assert(root.removeSession(&c, fallback));
    assert(fallback == &d);
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &b, &d}));
    assert(root.horizontalSpan() == 2);
    assert(root.verticalSpan() == 2);

    fallback = nullptr;
    assert(root.removeSession(&b, fallback));
    assert(fallback == &d);
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &d}));
    assert(root.horizontalSpan() == 2);
    assert(root.verticalSpan() == 1);

    fallback = nullptr;
    assert(root.removeSession(&d, fallback));
    assert(fallback == &a);
    assert(root.isLeaf());
    assert(root.session() == &a);
    assert(root.horizontalSpan() == 1);
    assert(root.verticalSpan() == 1);

    // Repeated splits in one direction must represent equal visual slots even
    // though the underlying data structure is a recursive binary tree.
    TerminalSession e;
    TerminalSession f;
    TerminalSession g;
    assert(root.splitSession(&a, Qt::Horizontal, &e));
    assert(root.splitSession(&e, Qt::Horizontal, &f));
    assert(root.splitSession(&f, Qt::Horizontal, &g));
    assert(root.horizontalSpan() == 4);
    assert(root.verticalSpan() == 1);

    // Closing the right-most pane should move focus to the leaf directly on
    // its left, not jump back to the first leaf of the surviving subtree.
    fallback = nullptr;
    assert(root.removeSession(&g, fallback));
    assert(fallback == &f);
    assert(root.leafSessions() == QVector<TerminalSession*>({&a, &e, &f}));
    assert(root.horizontalSpan() == 3);

    std::cout << "Split tree smoke OK\n";
    return 0;
}
