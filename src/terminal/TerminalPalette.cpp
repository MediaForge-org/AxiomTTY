#include "TerminalPalette.h"

#include "TerminalScreen.h"

TerminalPalette terminalPaletteForScheme(const QString& name)
{
    if (name == QStringLiteral("Midnight")) {
        return {
            QColor("#d5e4f2"), QColor("#07111f"),
            {QColor("#101c2c"), QColor("#ef7380"), QColor("#85d39a"), QColor("#efc56f"),
             QColor("#68aef5"), QColor("#c18bea"), QColor("#62c5d7"), QColor("#dbe7f1"),
             QColor("#61758b"), QColor("#ff8792"), QColor("#9be3ad"), QColor("#ffd483"),
             QColor("#82c0ff"), QColor("#d39cf5"), QColor("#7ad7e7"), QColor("#f5f9fc")},
            QColor("#76b5ff"), QColor("#244e78"), QColor("#f7fbff"),
            QColor("#324961"), QColor("#806726"), QColor("#f7fbff")
        };
    }

    if (name == QStringLiteral("Graphite")) {
        return {
            QColor("#dedede"), QColor("#151515"),
            {QColor("#222222"), QColor("#d8757a"), QColor("#93bb91"), QColor("#c9ab72"),
             QColor("#85a5cc"), QColor("#ad8fbb"), QColor("#83b3b6"), QColor("#cecece"),
             QColor("#6d6d6d"), QColor("#e88a90"), QColor("#a6cda4"), QColor("#d9bc82"),
             QColor("#98b8de"), QColor("#c0a0cc"), QColor("#96c5c8"), QColor("#f3f3f3")},
            QColor("#c0c9d3"), QColor("#484848"), QColor("#ffffff"),
            QColor("#444444"), QColor("#756136"), QColor("#ffffff")
        };
    }

    if (name == QStringLiteral("Forest")) {
        return {
            QColor("#d9eadc"), QColor("#07150c"),
            {QColor("#13231a"), QColor("#e17373"), QColor("#79cf91"), QColor("#d8bd6f"),
             QColor("#75aecd"), QColor("#b58ac7"), QColor("#68c0ad"), QColor("#d5e2d8"),
             QColor("#5f7868"), QColor("#f08787"), QColor("#90dfa5"), QColor("#e8cc80"),
             QColor("#8bc0dc"), QColor("#c69bd5"), QColor("#7bd0bc"), QColor("#f0f7f1")},
            QColor("#87dda0"), QColor("#285c39"), QColor("#f6fff7"),
            QColor("#31523b"), QColor("#75672f"), QColor("#f6fff7")
        };
    }

    return {
        QColor("#e6eaf0"), QColor("#0b0d10"),
        {QColor("#1b1f27"), QColor("#d96b73"), QColor("#72b783"), QColor("#d9ad6b"),
         QColor("#6f95d8"), QColor("#b47acb"), QColor("#65b8c5"), QColor("#d8dde6"),
         QColor("#66707d"), QColor("#ef7c85"), QColor("#83cb95"), QColor("#e8c07a"),
         QColor("#84a7e8"), QColor("#c68bdc"), QColor("#77cbd7"), QColor("#f4f6f8")},
        QColor("#86a9ff"), QColor("#2d4767"), QColor("#f4f7fb"),
        QColor("#39424f"), QColor("#756129"), QColor("#f4f7fb")
    };
}

QColor translateTerminalPaletteColor(const QColor& source, const TerminalPalette& palette)
{
    if (source == TerminalScreen::defaultForeground()) {
        return palette.foreground;
    }
    if (source == TerminalScreen::defaultBackground()) {
        return palette.background;
    }
    for (int index = 0; index < 16; ++index) {
        if (source == TerminalScreen::indexedColor(index)) {
            return palette.ansi.at(static_cast<std::size_t>(index));
        }
    }
    return source;
}
