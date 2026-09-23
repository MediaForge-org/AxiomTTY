#pragma once

#include <QColor>
#include <QString>

#include <array>

struct TerminalPalette
{
    QColor foreground;
    QColor background;
    std::array<QColor, 16> ansi;
    QColor cursor;
    QColor selectionBackground;
    QColor selectionForeground;
    QColor searchBackground;
    QColor searchCurrentBackground;
    QColor searchForeground;
};

[[nodiscard]] TerminalPalette terminalPaletteForScheme(const QString& name);
[[nodiscard]] QColor translateTerminalPaletteColor(const QColor& source, const TerminalPalette& palette);
