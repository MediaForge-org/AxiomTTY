#include "TerminalSearch.h"

#include "TerminalScreen.h"

#include <algorithm>
#include <vector>

namespace {
struct CharacterLocation
{
    int row{0};
    int column{0};
    int width{1};
};

void appendCellText(QString& text,
                    std::vector<CharacterLocation>& locations,
                    const TerminalCell& cell,
                    int row,
                    int column)
{
    const QString cellText = cell.text.isEmpty() ? QStringLiteral(" ") : cell.text;
    text += cellText;

    // QString::indexOf() reports UTF-16 offsets. Keep one location per UTF-16
    // code unit so search offsets can always be mapped back to terminal cells,
    // including combining sequences stored inside a single cell.
    for (qsizetype index = 0; index < cellText.size(); ++index) {
        locations.push_back(CharacterLocation{row, column, std::max(1, cell.width)});
    }
}

void findInLogicalLine(const QString& text,
                       const std::vector<CharacterLocation>& locations,
                       const QString& query,
                       Qt::CaseSensitivity caseSensitivity,
                       std::vector<TerminalSearchMatch>& matches)
{
    if (text.isEmpty() || query.isEmpty() || locations.empty()) {
        return;
    }

    qsizetype from = 0;
    while (from <= text.size() - query.size()) {
        const qsizetype index = text.indexOf(query, from, caseSensitivity);
        if (index < 0) {
            break;
        }

        const qsizetype endIndex = index + query.size() - 1;
        if (index < static_cast<qsizetype>(locations.size())
            && endIndex < static_cast<qsizetype>(locations.size())) {
            const CharacterLocation& start = locations.at(static_cast<std::size_t>(index));
            const CharacterLocation& end = locations.at(static_cast<std::size_t>(endIndex));
            matches.push_back(TerminalSearchMatch{
                start.row,
                start.column,
                end.row,
                end.column + std::max(1, end.width) - 1,
            });
        }

        // Allow overlapping matches (e.g. "ana" in "banana") while always
        // making forward progress.
        from = index + 1;
    }
}
}

std::vector<TerminalSearchMatch> findTerminalMatches(const TerminalScreen& screen,
                                                     const QString& query,
                                                     Qt::CaseSensitivity caseSensitivity)
{
    std::vector<TerminalSearchMatch> matches;
    if (query.isEmpty() || query.contains(QLatin1Char('\n')) || query.contains(QLatin1Char('\r'))) {
        return matches;
    }

    QString logicalText;
    std::vector<CharacterLocation> locations;

    const int totalRows = screen.historyRows();
    for (int rowIndex = 0; rowIndex < totalRows; ++rowIndex) {
        const TerminalScreen::Row& row = screen.historyRow(rowIndex);

        int lastContentColumn = -1;
        for (int column = screen.columns() - 1; column >= 0; --column) {
            const TerminalCell& cell = row.at(static_cast<std::size_t>(column));
            if (cell.continuation) {
                continue;
            }
            if ((!cell.text.isEmpty() && cell.text != QStringLiteral(" ")) || cell.softWrapAfter) {
                lastContentColumn = column;
                break;
            }
        }

        for (int column = 0; column <= lastContentColumn; ++column) {
            const TerminalCell& cell = row.at(static_cast<std::size_t>(column));
            if (cell.continuation) {
                continue;
            }
            appendCellText(logicalText, locations, cell, rowIndex, column);
        }

        const bool softWrapped = !row.empty() && row.back().softWrapAfter;
        if (!softWrapped) {
            findInLogicalLine(logicalText, locations, query, caseSensitivity, matches);
            logicalText.clear();
            locations.clear();
        }
    }

    if (!logicalText.isEmpty()) {
        findInLogicalLine(logicalText, locations, query, caseSensitivity, matches);
    }

    return matches;
}
