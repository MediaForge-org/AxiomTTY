#include "app/AppSettings.h"
#include "terminal/SessionManager.h"

#include <QCoreApplication>
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
    settings.setDefaultShell(QStringLiteral("/bin/sh"));
    settings.setStartDirectory(QStringLiteral("/tmp"));
    settings.setInheritWorkingDirectory(false);
    settings.setConfirmCloseRunningProcesses(false);

    SessionManager manager(&settings);
    assert(manager.count() == 1);
    assert(manager.activeSession() != nullptr);

    bool closeApproved = false;
    QObject::connect(&manager, &SessionManager::applicationCloseApproved, [&closeApproved] {
        closeApproved = true;
    });

    manager.closeTab(0);
    assert(manager.count() == 0);
    assert(manager.activeSession() == nullptr);
    assert(closeApproved);

    std::cout << "Session manager smoke OK\n";
    return 0;
}
