#include "terminal/TerminalScreen.h"
#include "terminal/VtParser.h"

#include <QCoreApplication>
#include <QDebug>

namespace {
bool expect(bool condition, const char* message)
{
    if (!condition) {
        qCritical() << "FAIL:" << message;
        return false;
    }
    return true;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    TerminalScreen screen(8, 40);
    VtParser parser(screen);

    parser.consume(QByteArray("hello\r\nworld"));
    if (!expect(screen.cell(0, 0).text == QStringLiteral("h"), "plain text")) return 1;
    if (!expect(screen.cell(1, 0).text == QStringLiteral("w"), "CRLF positioning")) return 1;

    parser.consume(QByteArray("\x1b[31mR\x1b[0m"));
    if (!expect(screen.cell(1, 5).text == QStringLiteral("R"), "SGR text")) return 1;
    if (!expect(screen.cell(1, 5).style.foreground == TerminalScreen::indexedColor(1), "SGR foreground")) return 1;

    parser.consume(QByteArray("\x1b[2J\x1b[Htop"));
    if (!expect(screen.cell(0, 0).text == QStringLiteral("t"), "clear + home")) return 1;

    parser.consume(QByteArray("\r\nsecond line"));
    if (!expect(screen.textInRange(0, 0, 0, 2) == QStringLiteral("top"), "single-line selection text")) return 1;
    if (!expect(screen.textInRange(0, 1, 1, 5) == QStringLiteral("op\nsecond"), "multi-line selection text")) return 1;

    parser.consume(QByteArray("\x1b[?25l"));
    if (!expect(!screen.cursorVisible(), "cursor visibility mode")) return 1;
    parser.consume(QByteArray("\x1b[?25h"));
    if (!expect(screen.cursorVisible(), "cursor visibility restore")) return 1;

    parser.consume(QByteArray("\x1b[?1049hALT"));
    if (!expect(screen.alternateScreenActive(), "alternate screen enter")) return 1;
    if (!expect(screen.cell(0, 0).text == QStringLiteral("A"), "alternate screen content")) return 1;
    parser.consume(QByteArray("\x1b[?1049l"));
    if (!expect(!screen.alternateScreenActive(), "alternate screen leave")) return 1;
    if (!expect(screen.cell(0, 0).text == QStringLiteral("t"), "primary screen restored")) return 1;

    TerminalScreen historyScreen(3, 16);
    VtParser historyParser(historyScreen);
    historyParser.consume(QByteArray("one\r\ntwo\r\nthree\r\nfour"));
    if (!expect(historyScreen.scrollbackRows() == 1, "scrollback collection")) return 1;
    if (!expect(historyScreen.historyRows() == 4, "history row count")) return 1;
    if (!expect(historyScreen.historyRow(0).at(0).text == QStringLiteral("o"), "scrollback row access")) return 1;
    if (!expect(historyScreen.textInHistoryRange(0, 0, 3, 4) == QStringLiteral("one\ntwo\nthree\nfour"), "history text extraction")) return 1;

    QByteArray response;
    parser.setResponseHandler([&response](const QByteArray& bytes) { response = bytes; });
    parser.consume(QByteArray("\x1b[6n"));
    if (!expect(response.startsWith("\x1b["), "cursor report response")) return 1;

    qInfo() << "VT core smoke OK";
    return 0;
}
