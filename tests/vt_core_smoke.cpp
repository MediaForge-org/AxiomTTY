#include "terminal/TerminalScreen.h"
#include "terminal/TerminalSearch.h"
#include "terminal/VtParser.h"

#include <QCoreApplication>
#include <QDebug>

#include <clocale>

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
    if (std::setlocale(LC_CTYPE, "C.UTF-8") == nullptr) {
        std::setlocale(LC_CTYPE, "");
    }

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

    parser.consume(QByteArray("\x1b[5 q"));
    if (!expect(screen.cursorShape() == TerminalCursorShape::Bar, "DECSCUSR bar cursor")) return 1;
    if (!expect(screen.cursorBlinking(), "DECSCUSR blinking cursor")) return 1;
    parser.consume(QByteArray("\x1b[4 q"));
    if (!expect(screen.cursorShape() == TerminalCursorShape::Underline, "DECSCUSR underline cursor")) return 1;
    if (!expect(!screen.cursorBlinking(), "DECSCUSR steady cursor")) return 1;

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

    TerminalScreen limitedHistoryScreen(2, 12);
    limitedHistoryScreen.setMaxScrollbackRows(100);
    for (int line = 0; line < 150; ++line) {
        limitedHistoryScreen.writeText(QStringLiteral("x\r\n"));
    }
    if (!expect(limitedHistoryScreen.maxScrollbackRows() == 100, "configurable scrollback limit")) return 1;
    if (!expect(limitedHistoryScreen.scrollbackRows() <= 100, "scrollback limit trimming")) return 1;

    TerminalScreen unicodeScreen(3, 12);
    unicodeScreen.writeText(QStringLiteral("A界e\u0301"));
    if (!expect(unicodeScreen.cell(0, 0).text == QStringLiteral("A"), "unicode ASCII lead")) return 1;
    if (!expect(unicodeScreen.cell(0, 1).text == QStringLiteral("界"), "wide glyph text")) return 1;
    if (!expect(unicodeScreen.cell(0, 1).width == 2, "wide glyph width")) return 1;
    if (!expect(unicodeScreen.cell(0, 2).continuation, "wide glyph continuation cell")) return 1;
    if (!expect(unicodeScreen.cell(0, 3).text == QStringLiteral("e\u0301"), "combining mark attachment")) return 1;
    if (!expect(unicodeScreen.cursorColumn() == 4, "unicode cursor width")) return 1;

    TerminalScreen wrapScreen(2, 4);
    wrapScreen.writeText(QStringLiteral("abcde"));
    if (!expect(wrapScreen.textInRange(0, 0, 1, 0) == QStringLiteral("abcde"), "soft-wrap-aware text extraction")) return 1;

    TerminalScreen searchScreen(3, 12);
    VtParser searchParser(searchScreen);
    searchParser.consume(QByteArray("alpha error\r\nbeta ERROR\r\ngamma error"));
    const auto insensitiveMatches = findTerminalMatches(searchScreen, QStringLiteral("error"), Qt::CaseInsensitive);
    if (!expect(insensitiveMatches.size() == 3, "scrollback search case-insensitive matches")) return 1;
    const auto sensitiveMatches = findTerminalMatches(searchScreen, QStringLiteral("error"), Qt::CaseSensitive);
    if (!expect(sensitiveMatches.size() == 2, "scrollback search case-sensitive matches")) return 1;

    TerminalScreen wrappedSearchScreen(2, 4);
    wrappedSearchScreen.writeText(QStringLiteral("abcde"));
    const auto wrappedMatches = findTerminalMatches(wrappedSearchScreen, QStringLiteral("cde"), Qt::CaseSensitive);
    if (!expect(wrappedMatches.size() == 1, "search crosses soft-wrapped terminal rows")) return 1;
    if (!expect(wrappedMatches.front().startRow == 0 && wrappedMatches.front().startColumn == 2, "wrapped search start coordinate")) return 1;
    if (!expect(wrappedMatches.front().endRow == 1 && wrappedMatches.front().endColumn == 0, "wrapped search end coordinate")) return 1;

    TerminalScreen unicodeSearchScreen(2, 12);
    unicodeSearchScreen.writeText(QStringLiteral("A界B界C"));
    const auto unicodeMatches = findTerminalMatches(unicodeSearchScreen, QStringLiteral("界B界"), Qt::CaseSensitive);
    if (!expect(unicodeMatches.size() == 1, "wide-character search match")) return 1;
    if (!expect(unicodeMatches.front().startColumn == 1 && unicodeMatches.front().endColumn == 5, "wide-character search coordinates")) return 1;

    TerminalScreen charsetScreen(2, 12);
    VtParser charsetParser(charsetScreen);
    charsetParser.consume(QByteArray("\x1b(0lqk\x1b(B"));
    if (!expect(charsetScreen.textInRange(0, 0, 0, 2) == QStringLiteral("┌─┐"), "DEC special graphics")) return 1;

    TerminalScreen modeScreen(4, 20);
    VtParser modeParser(modeScreen);
    modeParser.consume(QByteArray("\x1b[?1003;1006;1004h"));
    if (!expect(modeScreen.mouseTrackingMode() == TerminalMouseTrackingMode::AnyEvent, "mouse any-event mode")) return 1;
    if (!expect(modeScreen.sgrMouseMode(), "SGR mouse mode")) return 1;
    if (!expect(modeScreen.focusReporting(), "focus reporting mode")) return 1;
    modeParser.consume(QByteArray("\x1b[?1003;1006;1004l"));
    if (!expect(modeScreen.mouseTrackingMode() == TerminalMouseTrackingMode::None, "mouse mode disable")) return 1;
    if (!expect(!modeScreen.sgrMouseMode(), "SGR mouse disable")) return 1;
    if (!expect(!modeScreen.focusReporting(), "focus reporting disable")) return 1;

    modeParser.consume(QByteArray("\x1b[4h"));
    if (!expect(modeScreen.insertMode(), "ANSI insert mode")) return 1;
    modeParser.consume(QByteArray("\x1b[4l"));
    if (!expect(!modeScreen.insertMode(), "ANSI replace mode")) return 1;

    TerminalScreen resizeScreen(3, 10);
    VtParser resizeParser(resizeScreen);
    resizeParser.consume(QByteArray("one\r\ntwo\r\nthree"));
    resizeScreen.resize(2, 10);
    if (!expect(resizeScreen.scrollbackRows() == 1, "resize preserves removed top row in scrollback")) return 1;
    if (!expect(resizeScreen.row(0).at(0).text == QStringLiteral("t"), "resize keeps newest visible content")) return 1;
    resizeScreen.resize(3, 10);
    if (!expect(resizeScreen.scrollbackRows() == 0, "resize can restore recent history row")) return 1;
    if (!expect(resizeScreen.row(0).at(0).text == QStringLiteral("o"), "resize restores history at top")) return 1;

    TerminalScreen fontGrowScreen(3, 12);
    VtParser fontGrowParser(fontGrowScreen);
    fontGrowParser.consume(QByteArray("one\r\ntwo\r\nthree\r\nfour"));
    if (!expect(fontGrowScreen.scrollbackRows() == 1, "font-resize grow setup history")) return 1;
    fontGrowScreen.resize(5, 12, TerminalResizeMode::PreserveViewportTop);
    if (!expect(fontGrowScreen.scrollbackRows() == 1, "font-resize grow keeps scrollback in history")) return 1;
    if (!expect(fontGrowScreen.row(0).at(0).text == QStringLiteral("t"), "font-resize grow preserves viewport top")) return 1;
    if (!expect(fontGrowScreen.row(2).at(0).text == QStringLiteral("f"), "font-resize grow preserves prompt/content row")) return 1;
    if (!expect(fontGrowScreen.cursorRow() == 2, "font-resize grow keeps cursor logical row")) return 1;

    TerminalScreen fontShrinkScreen(5, 12);
    VtParser fontShrinkParser(fontShrinkScreen);
    fontShrinkParser.consume(QByteArray("one\r\ntwo"));
    fontShrinkScreen.resize(3, 12, TerminalResizeMode::PreserveViewportTop);
    if (!expect(fontShrinkScreen.scrollbackRows() == 0, "font-resize shrink trims unused bottom rows first")) return 1;
    if (!expect(fontShrinkScreen.row(0).at(0).text == QStringLiteral("o"), "font-resize shrink preserves first visible row")) return 1;
    if (!expect(fontShrinkScreen.row(1).at(0).text == QStringLiteral("t"), "font-resize shrink preserves current prompt row")) return 1;
    if (!expect(fontShrinkScreen.cursorRow() == 1, "font-resize shrink keeps cursor logical row when it fits")) return 1;

    TerminalScreen tabScreen(2, 24);
    VtParser tabParser(tabScreen);
    tabParser.consume(QByteArray("A\tB"));
    if (!expect(tabScreen.cell(0, 8).text == QStringLiteral("B"), "default tab stops every eight columns")) return 1;
    tabParser.consume(QByteArray("\x1b[3g\x1b[1;6H\x1bH\x1b[H\tT"));
    if (!expect(tabScreen.cell(0, 5).text == QStringLiteral("T"), "custom horizontal tab stop")) return 1;
    tabParser.consume(QByteArray("\x1b[3g\x1b[H\tZ"));
    if (!expect(tabScreen.cell(0, 23).text == QStringLiteral("Z"), "cleared tab stops fall through to last column")) return 1;

    TerminalScreen repeatScreen(2, 16);
    VtParser repeatParser(repeatScreen);
    repeatParser.consume(QByteArray("X\x1b[4b"));
    if (!expect(repeatScreen.textInRange(0, 0, 0, 4) == QStringLiteral("XXXXX"), "CSI REP repeats preceding graphic character")) return 1;

    TerminalScreen renditionScreen(2, 16);
    VtParser renditionParser(renditionScreen);
    renditionParser.consume(QByteArray("\x1b[2;9mD\x1b[22;29mN"));
    if (!expect(renditionScreen.cell(0, 0).style.faint, "SGR faint attribute")) return 1;
    if (!expect(renditionScreen.cell(0, 0).style.strikethrough, "SGR strikethrough attribute")) return 1;
    if (!expect(!renditionScreen.cell(0, 1).style.faint, "SGR faint reset")) return 1;
    if (!expect(!renditionScreen.cell(0, 1).style.strikethrough, "SGR strikethrough reset")) return 1;

    TerminalScreen syncScreen(2, 16);
    VtParser syncParser(syncScreen);
    syncParser.consume(QByteArray("\x1b[?2026h"));
    if (!expect(syncScreen.synchronizedOutput(), "synchronized output mode enable")) return 1;
    syncParser.consume(QByteArray("\x1b[?2026l"));
    if (!expect(!syncScreen.synchronizedOutput(), "synchronized output mode disable")) return 1;
    syncParser.consume(QByteArray("\x1b[?1007h"));
    if (!expect(syncScreen.alternateScroll(), "alternate-scroll mode enable")) return 1;
    syncParser.consume(QByteArray("\x1b[?1007l"));
    if (!expect(!syncScreen.alternateScroll(), "alternate-scroll mode disable")) return 1;

    TerminalScreen saveStateScreen(2, 16);
    VtParser saveStateParser(saveStateScreen);
    saveStateParser.consume(QByteArray("\x1b[31m\0337\x1b[32mX\0338Y"));
    if (!expect(saveStateScreen.cell(0, 0).text == QStringLiteral("Y"), "DECSC/DECRC cursor restore")) return 1;
    if (!expect(saveStateScreen.cell(0, 0).style.foreground == TerminalScreen::indexedColor(1), "DECSC/DECRC rendition restore")) return 1;

    TerminalScreen eraseHistoryScreen(2, 12);
    VtParser eraseHistoryParser(eraseHistoryScreen);
    eraseHistoryParser.consume(QByteArray("one\r\ntwo\r\nthree"));
    if (!expect(eraseHistoryScreen.scrollbackRows() == 1, "ED3 setup history")) return 1;
    eraseHistoryParser.consume(QByteArray("\x1b[3J"));
    if (!expect(eraseHistoryScreen.scrollbackRows() == 0, "ED3 clears scrollback")) return 1;

    QByteArray response;
    parser.setResponseHandler([&response](const QByteArray& bytes) { response = bytes; });
    parser.consume(QByteArray("\x1b[6n"));
    if (!expect(response.startsWith("\x1b["), "cursor report response")) return 1;

    response.clear();
    parser.consume(QByteArray("\x1b]10;?\a"));
    if (!expect(response.startsWith("\x1b]10;rgb:"), "OSC 10 foreground color query")) return 1;
    response.clear();
    parser.consume(QByteArray("\x1b]11;?\a"));
    if (!expect(response.startsWith("\x1b]11;rgb:"), "OSC 11 background color query")) return 1;

    qInfo() << "AxiomTTY VT core smoke OK";
    return 0;
}
