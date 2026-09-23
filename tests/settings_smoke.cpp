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
    assert(settings.confirmCloseRunningProcesses());
    assert(settings.profileNames().contains(QStringLiteral("Default")));
    assert(settings.profileNames().contains(QStringLiteral("Development")));
    assert(settings.profileNames().contains(QStringLiteral("Server")));
    assert(settings.activeProfile() == QStringLiteral("Default"));
    assert(settings.profileColorScheme(QStringLiteral("Development")) == QStringLiteral("Midnight"));
    assert(settings.profileColorScheme(QStringLiteral("Server")) == QStringLiteral("Graphite"));

    settings.setTerminalFontFamily(QStringLiteral("Test Mono"));
    settings.setTerminalFontSize(18);
    int profileThemeSignals = 0;
    QObject::connect(&settings, &AppSettings::profileColorSchemeChanged,
                     [&profileThemeSignals](const QString& profileName, const QString& colorScheme) {
        if (profileName != QStringLiteral("Default")) {
            return;
        }
        assert(colorScheme == QStringLiteral("Forest"));
        ++profileThemeSignals;
    });
    settings.setProfileSettings(QStringLiteral("Default"), QStringLiteral("/bin/sh"),
                                QStringLiteral("/tmp"), QStringLiteral("Forest"));
    assert(profileThemeSignals == 1);
    settings.setConfirmCloseRunningProcesses(false);

    assert(settings.createProfile(QStringLiteral("CI")));
    assert(!settings.createProfile(QStringLiteral("CI")));
    settings.setProfileShell(QStringLiteral("CI"), QStringLiteral("/bin/sh"));
    settings.setProfileStartDirectory(QStringLiteral("CI"), QStringLiteral("/tmp"));
    settings.setProfileColorScheme(QStringLiteral("CI"), QStringLiteral("Graphite"));
    settings.setActiveProfile(QStringLiteral("CI"));
    settings.flush();

    AppSettings persisted;
    assert(persisted.terminalFontFamily() == QStringLiteral("Test Mono"));
    assert(persisted.terminalFontSize() == 18);
    assert(persisted.defaultShell() == QStringLiteral("/bin/sh"));
    assert(persisted.startDirectory() == QStringLiteral("/tmp"));
    assert(!persisted.confirmCloseRunningProcesses());
    assert(persisted.profileNames().contains(QStringLiteral("CI")));
    assert(persisted.activeProfile() == QStringLiteral("CI"));
    assert(persisted.profileShell(QStringLiteral("CI")) == QStringLiteral("/bin/sh"));
    assert(persisted.profileStartDirectory(QStringLiteral("CI")) == QStringLiteral("/tmp"));
    assert(persisted.profileColorScheme(QStringLiteral("CI")) == QStringLiteral("Graphite"));

    persisted.setProfileShell(QStringLiteral("CI"), QStringLiteral("/tmp"));
    assert(persisted.profileShell(QStringLiteral("CI")) != QStringLiteral("/tmp"));
    persisted.setProfileStartDirectory(QStringLiteral("CI"), QStringLiteral("/definitely/not/a/real/path"));
    assert(persisted.profileStartDirectory(QStringLiteral("CI")) == QDir::homePath());
    persisted.setProfileColorScheme(QStringLiteral("CI"), QStringLiteral("Not A Scheme"));
    assert(persisted.profileColorScheme(QStringLiteral("CI")) == QStringLiteral("Axiom Dark"));

    assert(persisted.removeProfile(QStringLiteral("CI")));
    assert(!persisted.profileNames().contains(QStringLiteral("CI")));
    assert(persisted.activeProfile() == QStringLiteral("Default"));
    assert(!persisted.removeProfile(QStringLiteral("Default")));

    int resetSignals = 0;
    QObject::connect(&persisted, &AppSettings::defaultsReset, [&resetSignals]() { ++resetSignals; });
    persisted.resetDefaults();
    assert(resetSignals == 1);
    assert(persisted.activeProfile() == QStringLiteral("Default"));
    persisted.flush();

    std::cout << "Settings/profile smoke OK\n";
    return 0;
}
