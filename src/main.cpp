#include <QGuiApplication>

#include <clocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <qqml.h>

#include "terminal/TerminalSession.h"
#include "terminal/TerminalView.h"

int main(int argc, char* argv[])
{
    std::setlocale(LC_CTYPE, "");
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("AxiomTTY"));
    QGuiApplication::setOrganizationName(QStringLiteral("MediaForge"));

    qmlRegisterType<TerminalView>("AxiomTTY.Native", 1, 0, "TerminalView");

    TerminalSession terminalSession;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("terminalSession"), &terminalSession);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule("AxiomTTY", "Main");

    QTimer::singleShot(0, &terminalSession, &TerminalSession::startDefaultShell);

    return app.exec();
}
