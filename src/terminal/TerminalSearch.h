#pragma once

#include <QString>
#include <QtCore/qnamespace.h>

#include <vector>

class TerminalScreen;

struct TerminalSearchMatch
{
    int startRow{0};
    int startColumn{0};
    int endRow{0};
    int endColumn{0};

    friend bool operator==(const TerminalSearchMatch&, const TerminalSearchMatch&) = default;
};

[[nodiscard]] std::vector<TerminalSearchMatch> findTerminalMatches(
    const TerminalScreen& screen,
    const QString& query,
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);
