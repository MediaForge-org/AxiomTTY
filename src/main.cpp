#include <QGuiApplication>

#include <clocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <qqml.h>

#include "terminal/SessionManager.h"
#include "terminal/TerminalView.h"

int main(int argc, char* argv[])
{
    std::setlocale(LC_CTYPE, "");
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("AxiomTTY"));
    QGuiApplication::setOrganizationName(QStringLiteral("MediaForge"));

    qmlRegisterType<TerminalView>("AxiomTTY.Native", 1, 0, "TerminalView");

    SessionManager sessionManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("sessions"), &sessionManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule("AxiomTTY", "Main");


    return app.exec();
}
