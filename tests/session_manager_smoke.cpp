#include "app/AppSettings.h"
#include "terminal/SessionManager.h"
#include "terminal/TerminalSession.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MediaForgeTest"));
    QCoreApplication::setApplicationName(QStringLiteral("AxiomTTYSessionManagerSmoke"));
    QStandardPaths::setTestModeEnabled(true);

    AppSettings settings;
    settings.resetDefaults();
    settings.setProfileShell(QStringLiteral("Default"), QStringLiteral("/bin/sh"));
    settings.setProfileStartDirectory(QStringLiteral("Default"), QDir::homePath());
    settings.setConfirmCloseRunningProcesses(false);

    SessionManager manager(&settings);
    assert(manager.count() == 1);
    auto* first = qobject_cast<TerminalSession*>(manager.activeSession());
    assert(first != nullptr);
    assert(first->profileName() == QStringLiteral("Default"));

    // Even a same-profile fresh tab must use the profile start directory,
    // not the active pane's current CWD.
    first->setInitialWorkingDirectory(QStringLiteral("/tmp"));
    manager.newTab();
    assert(manager.count() == 2);
    auto* freshDefault = qobject_cast<TerminalSession*>(manager.activeSession());
    assert(freshDefault != nullptr);
    assert(freshDefault->profileName() == QStringLiteral("Default"));
    assert(freshDefault->workingDirectory() == QDir::homePath());

    settings.setProfileStartDirectory(QStringLiteral("Development"), QStringLiteral("/tmp"));
    settings.setActiveProfile(QStringLiteral("Development"));
    manager.newTab();
    assert(manager.count() == 3);
    auto* development = qobject_cast<TerminalSession*>(manager.activeSession());
    assert(development != nullptr);
    assert(development->profileName() == QStringLiteral("Development"));
    assert(development->workingDirectory() == QStringLiteral("/tmp"));
    assert(development->colorScheme() == QStringLiteral("Midnight"));

    settings.setProfileColorScheme(QStringLiteral("Development"), QStringLiteral("Forest"));
    assert(development->colorScheme() == QStringLiteral("Forest"));

    // Splits are intentionally bounded per tab. Repeated split shortcuts must
    // never grow a tab beyond the public limit.
    for (int index = 0; index < manager.maxPanesPerTab() + 4; ++index) {
        manager.splitRight();
    }
    assert(manager.activePaneCount() == manager.maxPanesPerTab());
    assert(!manager.canSplitActivePane());

    // Collapse back to one pane so the existing tab lifecycle checks remain
    // straightforward and also exercise repeated split-tree promotion.
    while (manager.activePaneCount() > 1) {
        manager.closeActivePane();
    }
    assert(manager.activePaneCount() == 1);
    assert(manager.canSplitActivePane());

    bool closeApproved = false;
    QObject::connect(&manager, &SessionManager::applicationCloseApproved, [&closeApproved] {
        closeApproved = true;
    });

    manager.closeTab(2);
    assert(manager.count() == 2);
    manager.closeTab(1);
    assert(manager.count() == 1);
    manager.closeTab(0);
    assert(manager.count() == 0);
    assert(manager.activeSession() == nullptr);
    assert(closeApproved);

    std::cout << "Session manager/profile smoke OK\n";
    return 0;
}
