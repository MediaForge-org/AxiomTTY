#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <qqml.h>

#include "terminal/TerminalSession.h"
#include "terminal/TerminalView.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("TerminalCpp"));
    QGuiApplication::setOrganizationName(QStringLiteral("TerminalCpp"));

    qmlRegisterType<TerminalView>("TerminalCpp.Native", 1, 0, "TerminalView");

    TerminalSession terminalSession;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("terminalSession"), &terminalSession);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule("TerminalCpp", "Main");

    QTimer::singleShot(0, &terminalSession, &TerminalSession::startDefaultShell);

    return app.exec();
}
