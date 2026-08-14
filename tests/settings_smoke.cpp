#include "app/AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MediaForgeTest"));
    QCoreApplication::setApplicationName(QStringLiteral("AxiomTTYSettingsSmoke"));
    QStandardPaths::setTestModeEnabled(true);

    AppSettings settings;
    settings.resetDefaults();

    assert(settings.terminalFontSize() == 14);
    assert(settings.inheritWorkingDirectory());
    assert(settings.confirmCloseRunningProcesses());

    settings.setTerminalFontFamily(QStringLiteral("Test Mono"));
    settings.setTerminalFontSize(18);
    settings.setDefaultShell(QStringLiteral("/bin/sh"));
    settings.setStartDirectory(QStringLiteral("/tmp"));
    settings.setInheritWorkingDirectory(false);
    settings.setConfirmCloseRunningProcesses(false);
    settings.flush();

    AppSettings persisted;
    assert(persisted.terminalFontFamily() == QStringLiteral("Test Mono"));
    assert(persisted.terminalFontSize() == 18);
    assert(persisted.defaultShell() == QStringLiteral("/bin/sh"));
    assert(persisted.startDirectory() == QStringLiteral("/tmp"));
    assert(!persisted.inheritWorkingDirectory());
    assert(!persisted.confirmCloseRunningProcesses());

    persisted.setDefaultShell(QStringLiteral("/tmp"));
    assert(persisted.defaultShell() != QStringLiteral("/tmp"));
    persisted.setStartDirectory(QStringLiteral("/definitely/not/a/real/path"));
    assert(persisted.startDirectory() == QDir::homePath());

    int resetSignals = 0;
    QObject::connect(&persisted, &AppSettings::defaultsReset, [&resetSignals]() { ++resetSignals; });
    persisted.resetDefaults();
    assert(resetSignals == 1);
    persisted.flush();
    std::cout << "Settings smoke OK\n";
    return 0;
}
