#include "TerminalScreen.h"

#include <QChar>

#include <algorithm>
#include <array>
#include <wchar.h>

namespace {
constexpr int clampPositiveCount(int count)
{
    return std::max(1, count);
}
}

TerminalScreen::TerminalScreen(int rows, int columns)
    : m_rows(std::max(1, rows))
    , m_columns(std::max(1, columns))
    , m_scrollBottom(m_rows - 1)
{
    m_primaryRows.assign(static_cast<std::size_t>(m_rows), blankRow());
    m_alternateRows.assign(static_cast<std::size_t>(m_rows), blankRow());
    resetTabStops();
}

int TerminalScreen::rows() const noexcept { return m_rows; }
int TerminalScreen::columns() const noexcept { return m_columns; }
int TerminalScreen::cursorRow() const noexcept { return m_cursorRow; }
int TerminalScreen::cursorColumn() const noexcept { return m_cursorColumn; }
bool TerminalScreen::cursorVisible() const noexcept { return m_cursorVisible; }
TerminalCursorShape TerminalScreen::cursorShape() const noexcept { return m_cursorShape; }
bool TerminalScreen::cursorBlinking() const noexcept { return m_cursorBlinking; }
bool TerminalScreen::bracketedPaste() const noexcept { return m_bracketedPaste; }
bool TerminalScreen::applicationCursorKeys() const noexcept { return m_applicationCursorKeys; }
bool TerminalScreen::alternateScreenActive() const noexcept { return m_alternateScreen; }
bool TerminalScreen::originMode() const noexcept { return m_originMode; }
bool TerminalScreen::insertMode() const noexcept { return m_insertMode; }
TerminalMouseTrackingMode TerminalScreen::mouseTrackingMode() const noexcept { return m_mouseTrackingMode; }
bool TerminalScreen::sgrMouseMode() const noexcept { return m_sgrMouseMode; }
bool TerminalScreen::focusReporting() const noexcept { return m_focusReporting; }
bool TerminalScreen::alternateScroll() const noexcept { return m_alternateScroll; }
bool TerminalScreen::synchronizedOutput() const noexcept { return m_synchronizedOutput; }
int TerminalScreen::scrollTop() const noexcept { return m_scrollTop; }
int TerminalScreen::scrollBottom() const noexcept { return m_scrollBottom; }
int TerminalScreen::scrollbackRows() const noexcept
{
    return m_alternateScreen ? 0 : static_cast<int>(m_scrollback.size());
}
int TerminalScreen::historyRows() const noexcept { return scrollbackRows() + m_rows; }

const TerminalScreen::Row& TerminalScreen::row(int index) const
{
    return activeRows().at(static_cast<std::size_t>(std::clamp(index, 0, m_rows - 1)));
}

const TerminalScreen::Row& TerminalScreen::historyRow(int index) const
{
    if (m_alternateScreen) {
        return row(index);
    }

    const int clamped = std::clamp(index, 0, historyRows() - 1);
    const int scrollbackCount = scrollbackRows();
    if (clamped < scrollbackCount) {
        return m_scrollback.at(static_cast<std::size_t>(clamped));
    }
    return m_primaryRows.at(static_cast<std::size_t>(clamped - scrollbackCount));
}

const TerminalCell& TerminalScreen::cell(int rowIndex, int columnIndex) const
{
    return row(rowIndex).at(static_cast<std::size_t>(std::clamp(columnIndex, 0, m_columns - 1)));
}

QString TerminalScreen::textInRange(int startRow, int startColumn, int endRow, int endColumn) const
{
    if (m_rows <= 0 || m_columns <= 0) {
        return {};
    }

    startRow = std::clamp(startRow, 0, m_rows - 1);
    endRow = std::clamp(endRow, 0, m_rows - 1);
    startColumn = std::clamp(startColumn, 0, m_columns - 1);
    endColumn = std::clamp(endColumn, 0, m_columns - 1);

    if (startRow > endRow || (startRow == endRow && startColumn > endColumn)) {
        std::swap(startRow, endRow);
        std::swap(startColumn, endColumn);
    }

    QString result;
    for (int rowIndex = startRow; rowIndex <= endRow; ++rowIndex) {
        const int firstColumn = rowIndex == startRow ? startColumn : 0;
        const int lastColumn = rowIndex == endRow ? endColumn : m_columns - 1;
        const Row& sourceRow = row(rowIndex);

        QString line;
        for (int columnIndex = firstColumn; columnIndex <= lastColumn; ++columnIndex) {
            const TerminalCell& sourceCell = sourceRow.at(static_cast<std::size_t>(columnIndex));
            if (sourceCell.continuation) {
                continue;
            }
            line += sourceCell.text.isEmpty() ? QStringLiteral(" ") : sourceCell.text;
        }

        while (line.endsWith(QLatin1Char(' '))) {
            line.chop(1);
        }

        result += line;
        const bool softWrapped = !sourceRow.empty() && sourceRow.back().softWrapAfter;
        if (rowIndex != endRow && !softWrapped) {
            result += QLatin1Char('\n');
        }
    }

    return result;
}

QString TerminalScreen::textInHistoryRange(int startRow, int startColumn, int endRow, int endColumn) const
{
    const int totalRows = historyRows();
    if (totalRows <= 0 || m_columns <= 0) {
        return {};
    }

    startRow = std::clamp(startRow, 0, totalRows - 1);
    endRow = std::clamp(endRow, 0, totalRows - 1);
    startColumn = std::clamp(startColumn, 0, m_columns - 1);
    endColumn = std::clamp(endColumn, 0, m_columns - 1);

    if (startRow > endRow || (startRow == endRow && startColumn > endColumn)) {
        std::swap(startRow, endRow);
        std::swap(startColumn, endColumn);
    }

    QString result;
    for (int rowIndex = startRow; rowIndex <= endRow; ++rowIndex) {
        const int firstColumn = rowIndex == startRow ? startColumn : 0;
        const int lastColumn = rowIndex == endRow ? endColumn : m_columns - 1;
        const Row& sourceRow = historyRow(rowIndex);

        QString line;
        for (int columnIndex = firstColumn; columnIndex <= lastColumn; ++columnIndex) {
            const TerminalCell& sourceCell = sourceRow.at(static_cast<std::size_t>(columnIndex));
            if (sourceCell.continuation) {
                continue;
            }
            line += sourceCell.text.isEmpty() ? QStringLiteral(" ") : sourceCell.text;
        }

        while (line.endsWith(QLatin1Char(' '))) {
            line.chop(1);
        }

        result += line;
        const bool softWrapped = !sourceRow.empty() && sourceRow.back().softWrapAfter;
        if (rowIndex != endRow && !softWrapped) {
            result += QLatin1Char('\n');
        }
    }

    return result;
}

void TerminalScreen::reset()
{
    m_cursorRow = 0;
    m_cursorColumn = 0;
    m_savedCursorRow = 0;
    m_savedCursorColumn = 0;
    m_cursorVisible = true;
    m_cursorShape = TerminalCursorShape::Block;
    m_cursorBlinking = true;
    m_bracketedPaste = false;
    m_applicationCursorKeys = false;
    m_autoWrap = true;
    m_originMode = false;
    m_insertMode = false;
    m_mouseTrackingMode = TerminalMouseTrackingMode::None;
    m_sgrMouseMode = false;
    m_focusReporting = false;
    m_alternateScroll = false;
    m_synchronizedOutput = false;
    m_wrapPending = false;
    m_alternateScreen = false;
    m_primaryCursorRow = 0;
    m_primaryCursorColumn = 0;
    m_currentStyle = TerminalCellStyle{};
    m_savedStyle = TerminalCellStyle{};
    m_savedAutoWrap = true;
    m_savedOriginMode = false;
    m_lastPrintedCodePoint = U'\0';
    resetMargins();
    resetTabStops();
    m_scrollback.clear();
    m_primaryRows.assign(static_cast<std::size_t>(m_rows), blankRow());
    m_alternateRows.assign(static_cast<std::size_t>(m_rows), blankRow());
}

void TerminalScreen::resize(int rowsValue, int columnsValue)
{
    const int newRows = std::max(1, rowsValue);
    const int newColumns = std::max(1, columnsValue);
    if (newRows == m_rows && newColumns == m_columns) {
        return;
    }

    const int oldRows = m_rows;
    const int oldColumns = m_columns;
    const int rowDelta = newRows - oldRows;
    m_columns = newColumns;

    const std::size_t previousTabCount = m_tabStops.size();
    m_tabStops.resize(static_cast<std::size_t>(newColumns), false);
    if (newColumns > oldColumns) {
        const int firstNewColumn = std::max(oldColumns, static_cast<int>(previousTabCount));
        for (int column = firstNewColumn; column < newColumns; ++column) {
            if (column > 0 && column % 8 == 0) {
                m_tabStops[static_cast<std::size_t>(column)] = true;
            }
        }
    }

    auto resizeColumns = [&](Row& rowValue) {
        rowValue.resize(static_cast<std::size_t>(newColumns), blankCell());
        sanitizeWideCells(rowValue);
    };

    for (Row& rowValue : m_primaryRows) {
        resizeColumns(rowValue);
    }
    for (Row& rowValue : m_alternateRows) {
        resizeColumns(rowValue);
    }
    for (Row& rowValue : m_scrollback) {
        resizeColumns(rowValue);
    }

    if (rowDelta < 0) {
        const int removeCount = -rowDelta;
        for (int index = 0; index < removeCount && !m_primaryRows.empty(); ++index) {
            m_scrollback.push_back(m_primaryRows.front());
            m_primaryRows.erase(m_primaryRows.begin());
        }
        while (m_scrollback.size() > MaxScrollbackRows) {
            m_scrollback.pop_front();
        }

        if (m_alternateScreen) {
            m_primaryCursorRow = std::max(0, m_primaryCursorRow - removeCount);
        } else {
            m_cursorRow = std::max(0, m_cursorRow - removeCount);
        }

        if (static_cast<int>(m_alternateRows.size()) > newRows) {
            m_alternateRows.resize(static_cast<std::size_t>(newRows));
        }
    } else if (rowDelta > 0) {
        const int pullCount = std::min(rowDelta, static_cast<int>(m_scrollback.size()));
        if (pullCount > 0) {
            std::vector<Row> restored;
            restored.reserve(static_cast<std::size_t>(pullCount));
            const auto first = m_scrollback.end() - pullCount;
            restored.insert(restored.end(), first, m_scrollback.end());
            m_scrollback.erase(first, m_scrollback.end());
            m_primaryRows.insert(m_primaryRows.begin(), restored.begin(), restored.end());

            if (m_alternateScreen) {
                m_primaryCursorRow += pullCount;
            } else {
                m_cursorRow += pullCount;
            }
        }

        while (static_cast<int>(m_primaryRows.size()) < newRows) {
            m_primaryRows.push_back(blankRow());
        }
        while (static_cast<int>(m_alternateRows.size()) < newRows) {
            m_alternateRows.push_back(blankRow());
        }
    }

    if (static_cast<int>(m_primaryRows.size()) > newRows) {
        m_primaryRows.resize(static_cast<std::size_t>(newRows));
    }
    while (static_cast<int>(m_primaryRows.size()) < newRows) {
        m_primaryRows.push_back(blankRow());
    }
    if (static_cast<int>(m_alternateRows.size()) > newRows) {
        m_alternateRows.resize(static_cast<std::size_t>(newRows));
    }
    while (static_cast<int>(m_alternateRows.size()) < newRows) {
        m_alternateRows.push_back(blankRow());
    }

    m_rows = newRows;
    resetMargins();
    clampCursor();
    m_primaryCursorRow = std::clamp(m_primaryCursorRow, 0, m_rows - 1);
    m_primaryCursorColumn = std::clamp(m_primaryCursorColumn, 0, m_columns - 1);
}

void TerminalScreen::writeText(const QString& text)
{
    const QList<uint> codePoints = text.toUcs4();
    for (const uint codePoint : codePoints) {
        putCodePoint(static_cast<char32_t>(codePoint));
    }
}

void TerminalScreen::carriageReturn()
{
    m_cursorColumn = 0;
    m_wrapPending = false;
}

void TerminalScreen::lineFeed()
{
    m_wrapPending = false;
    if (m_cursorRow == m_scrollBottom) {
        scrollRegionUp(1, !m_alternateScreen && m_scrollTop == 0);
        return;
    }
    m_cursorRow = std::min(m_rows - 1, m_cursorRow + 1);
}

void TerminalScreen::backspace()
{
    m_wrapPending = false;
    m_cursorColumn = std::max(0, m_cursorColumn - 1);
}

void TerminalScreen::horizontalTab()
{
    cursorForwardTab(1);
}

void TerminalScreen::cursorForwardTab(int count)
{
    m_wrapPending = false;
    int remaining = clampPositiveCount(count);
    int column = m_cursorColumn;
    while (remaining-- > 0) {
        int next = m_columns - 1;
        for (int candidate = column + 1; candidate < m_columns; ++candidate) {
            if (candidate < static_cast<int>(m_tabStops.size())
                && m_tabStops[static_cast<std::size_t>(candidate)]) {
                next = candidate;
                break;
            }
        }
        column = next;
    }
    m_cursorColumn = std::clamp(column, 0, m_columns - 1);
}

void TerminalScreen::cursorBackwardTab(int count)
{
    m_wrapPending = false;
    int remaining = clampPositiveCount(count);
    int column = m_cursorColumn;
    while (remaining-- > 0) {
        int previous = 0;
        for (int candidate = column - 1; candidate >= 0; --candidate) {
            if (candidate < static_cast<int>(m_tabStops.size())
                && m_tabStops[static_cast<std::size_t>(candidate)]) {
                previous = candidate;
                break;
            }
        }
        column = previous;
    }
    m_cursorColumn = std::clamp(column, 0, m_columns - 1);
}

void TerminalScreen::setTabStopAtCursor()
{
    if (m_cursorColumn >= 0 && m_cursorColumn < static_cast<int>(m_tabStops.size())) {
        m_tabStops[static_cast<std::size_t>(m_cursorColumn)] = true;
    }
}

void TerminalScreen::clearTabStopAtCursor()
{
    if (m_cursorColumn >= 0 && m_cursorColumn < static_cast<int>(m_tabStops.size())) {
        m_tabStops[static_cast<std::size_t>(m_cursorColumn)] = false;
    }
}

void TerminalScreen::clearAllTabStops()
{
    std::fill(m_tabStops.begin(), m_tabStops.end(), false);
}

void TerminalScreen::repeatLastCharacter(int count)
{
    if (m_lastPrintedCodePoint == U'\0') {
        return;
    }
    const char32_t repeated = m_lastPrintedCodePoint;
    for (int index = 0; index < clampPositiveCount(count); ++index) {
        putCodePoint(repeated);
    }
}

void TerminalScreen::cursorUp(int count)
{
    m_wrapPending = false;
    m_cursorRow = std::max(m_scrollTop, m_cursorRow - clampPositiveCount(count));
}

void TerminalScreen::cursorDown(int count)
{
    m_wrapPending = false;
    m_cursorRow = std::min(m_scrollBottom, m_cursorRow + clampPositiveCount(count));
}

void TerminalScreen::cursorForward(int count)
{
    m_wrapPending = false;
    m_cursorColumn = std::min(m_columns - 1, m_cursorColumn + clampPositiveCount(count));
}

void TerminalScreen::cursorBackward(int count)
{
    m_wrapPending = false;
    m_cursorColumn = std::max(0, m_cursorColumn - clampPositiveCount(count));
}

void TerminalScreen::cursorNextLine(int count)
{
    cursorDown(count);
    m_cursorColumn = 0;
}

void TerminalScreen::cursorPreviousLine(int count)
{
    cursorUp(count);
    m_cursorColumn = 0;
}

void TerminalScreen::setCursorPosition(int rowValue, int columnValue)
{
    m_wrapPending = false;
    m_cursorRow = std::clamp(rowValue, 0, m_rows - 1);
    m_cursorColumn = std::clamp(columnValue, 0, m_columns - 1);
}

void TerminalScreen::setCursorRow(int rowValue)
{
    setCursorPosition(rowValue, m_cursorColumn);
}

void TerminalScreen::setCursorColumn(int columnValue)
{
    setCursorPosition(m_cursorRow, columnValue);
}

void TerminalScreen::saveCursor()
{
    m_savedCursorRow = m_cursorRow;
    m_savedCursorColumn = m_cursorColumn;
    m_savedStyle = m_currentStyle;
    m_savedAutoWrap = m_autoWrap;
    m_savedOriginMode = m_originMode;
}

void TerminalScreen::restoreCursor()
{
    m_currentStyle = m_savedStyle;
    m_autoWrap = m_savedAutoWrap;
    m_originMode = m_savedOriginMode;
    setCursorPosition(m_savedCursorRow, m_savedCursorColumn);
}

void TerminalScreen::eraseDisplay(int mode)
{
    auto& rowsBuffer = activeRows();
    if (mode == 2 || mode == 3) {
        for (Row& currentRow : rowsBuffer) {
            std::fill(currentRow.begin(), currentRow.end(), blankCell());
        }
        if (mode == 3) {
            m_scrollback.clear();
        }
        return;
    }

    if (mode == 0) {
        eraseLine(0);
        for (int rowIndex = m_cursorRow + 1; rowIndex < m_rows; ++rowIndex) {
            std::fill(rowsBuffer[static_cast<std::size_t>(rowIndex)].begin(),
                      rowsBuffer[static_cast<std::size_t>(rowIndex)].end(), blankCell());
        }
        return;
    }

    if (mode == 1) {
        eraseLine(1);
        for (int rowIndex = 0; rowIndex < m_cursorRow; ++rowIndex) {
            std::fill(rowsBuffer[static_cast<std::size_t>(rowIndex)].begin(),
                      rowsBuffer[static_cast<std::size_t>(rowIndex)].end(), blankCell());
        }
    }
}

void TerminalScreen::eraseLine(int mode)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    if (mode == 2) {
        std::fill(currentRow.begin(), currentRow.end(), blankCell());
    } else if (mode == 1) {
        std::fill(currentRow.begin(), currentRow.begin() + m_cursorColumn + 1, blankCell());
    } else {
        std::fill(currentRow.begin() + m_cursorColumn, currentRow.end(), blankCell());
    }
    sanitizeWideCells(currentRow);
}

void TerminalScreen::eraseCharacters(int count)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    const int end = std::min(m_columns, m_cursorColumn + clampPositiveCount(count));
    std::fill(currentRow.begin() + m_cursorColumn, currentRow.begin() + end, blankCell());
    sanitizeWideCells(currentRow);
}

void TerminalScreen::insertCharacters(int count)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    const int amount = std::min(clampPositiveCount(count), m_columns - m_cursorColumn);
    std::move_backward(currentRow.begin() + m_cursorColumn,
                       currentRow.end() - amount,
                       currentRow.end());
    std::fill(currentRow.begin() + m_cursorColumn,
              currentRow.begin() + m_cursorColumn + amount,
              blankCell());
    sanitizeWideCells(currentRow);
}

void TerminalScreen::deleteCharacters(int count)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    const int amount = std::min(clampPositiveCount(count), m_columns - m_cursorColumn);
    std::move(currentRow.begin() + m_cursorColumn + amount,
              currentRow.end(),
              currentRow.begin() + m_cursorColumn);
    std::fill(currentRow.end() - amount, currentRow.end(), blankCell());
    sanitizeWideCells(currentRow);
}

void TerminalScreen::insertLines(int count)
{
    if (m_cursorRow < m_scrollTop || m_cursorRow > m_scrollBottom) {
        return;
    }
    auto& rowsBuffer = activeRows();
    const int amount = std::min(clampPositiveCount(count), m_scrollBottom - m_cursorRow + 1);
    for (int i = 0; i < amount; ++i) {
        rowsBuffer.insert(rowsBuffer.begin() + m_cursorRow, blankRow());
        rowsBuffer.erase(rowsBuffer.begin() + m_scrollBottom + 1);
    }
}

void TerminalScreen::deleteLines(int count)
{
    if (m_cursorRow < m_scrollTop || m_cursorRow > m_scrollBottom) {
        return;
    }
    auto& rowsBuffer = activeRows();
    const int amount = std::min(clampPositiveCount(count), m_scrollBottom - m_cursorRow + 1);
    for (int i = 0; i < amount; ++i) {
        rowsBuffer.erase(rowsBuffer.begin() + m_cursorRow);
        rowsBuffer.insert(rowsBuffer.begin() + m_scrollBottom, blankRow());
    }
}

void TerminalScreen::scrollUp(int count)
{
    scrollRegionUp(clampPositiveCount(count), false);
}

void TerminalScreen::scrollDown(int count)
{
    scrollRegionDown(clampPositiveCount(count));
}

void TerminalScreen::setScrollRegion(int top, int bottom)
{
    if (top < 0 || bottom < 0 || top >= bottom || bottom >= m_rows) {
        resetMargins();
    } else {
        m_scrollTop = top;
        m_scrollBottom = bottom;
    }
    setCursorPosition(m_originMode ? m_scrollTop : 0, 0);
}

void TerminalScreen::resetStyle() { m_currentStyle = TerminalCellStyle{}; }
void TerminalScreen::setBold(bool enabled) { m_currentStyle.bold = enabled; }
void TerminalScreen::setItalic(bool enabled) { m_currentStyle.italic = enabled; }
void TerminalScreen::setUnderline(bool enabled) { m_currentStyle.underline = enabled; }
void TerminalScreen::setFaint(bool enabled) { m_currentStyle.faint = enabled; }
void TerminalScreen::setStrikethrough(bool enabled) { m_currentStyle.strikethrough = enabled; }
void TerminalScreen::setInverse(bool enabled) { m_currentStyle.inverse = enabled; }
void TerminalScreen::setForeground(const QColor& color) { m_currentStyle.foreground = color; }
void TerminalScreen::setBackground(const QColor& color) { m_currentStyle.background = color; }
void TerminalScreen::setDefaultForeground() { m_currentStyle.foreground = defaultForeground(); }
void TerminalScreen::setDefaultBackground() { m_currentStyle.background = defaultBackground(); }
void TerminalScreen::setCursorVisible(bool visible) { m_cursorVisible = visible; }

void TerminalScreen::setCursorStyle(int styleCode)
{
    switch (styleCode) {
    case 0:
    case 1: m_cursorShape = TerminalCursorShape::Block; m_cursorBlinking = true; break;
    case 2: m_cursorShape = TerminalCursorShape::Block; m_cursorBlinking = false; break;
    case 3: m_cursorShape = TerminalCursorShape::Underline; m_cursorBlinking = true; break;
    case 4: m_cursorShape = TerminalCursorShape::Underline; m_cursorBlinking = false; break;
    case 5: m_cursorShape = TerminalCursorShape::Bar; m_cursorBlinking = true; break;
    case 6: m_cursorShape = TerminalCursorShape::Bar; m_cursorBlinking = false; break;
    default: break;
    }
}

void TerminalScreen::setCursorBlinking(bool enabled) { m_cursorBlinking = enabled; }
void TerminalScreen::setBracketedPaste(bool enabled) { m_bracketedPaste = enabled; }
void TerminalScreen::setApplicationCursorKeys(bool enabled) { m_applicationCursorKeys = enabled; }
void TerminalScreen::setAutoWrap(bool enabled) { m_autoWrap = enabled; }
void TerminalScreen::setOriginMode(bool enabled)
{
    m_originMode = enabled;
    setCursorPosition(enabled ? m_scrollTop : 0, 0);
}
void TerminalScreen::setInsertMode(bool enabled) { m_insertMode = enabled; }
void TerminalScreen::setMouseTrackingMode(TerminalMouseTrackingMode mode) { m_mouseTrackingMode = mode; }
void TerminalScreen::setSgrMouseMode(bool enabled) { m_sgrMouseMode = enabled; }
void TerminalScreen::setFocusReporting(bool enabled) { m_focusReporting = enabled; }
void TerminalScreen::setAlternateScroll(bool enabled) { m_alternateScroll = enabled; }
void TerminalScreen::setSynchronizedOutput(bool enabled) { m_synchronizedOutput = enabled; }

void TerminalScreen::useAlternateScreen(bool enabled, bool clearOnEnter)
{
    if (enabled == m_alternateScreen) {
        return;
    }

    if (enabled) {
        m_primaryCursorRow = m_cursorRow;
        m_primaryCursorColumn = m_cursorColumn;
        m_alternateScreen = true;
        if (clearOnEnter) {
            m_alternateRows.assign(static_cast<std::size_t>(m_rows), blankRow());
        }
        setCursorPosition(0, 0);
    } else {
        m_alternateScreen = false;
        setCursorPosition(m_primaryCursorRow, m_primaryCursorColumn);
    }
    resetMargins();
}

QColor TerminalScreen::defaultForeground() { return QColor(QStringLiteral("#e6eaf0")); }
QColor TerminalScreen::defaultBackground() { return QColor(QStringLiteral("#0b0d10")); }

QColor TerminalScreen::indexedColor(int index)
{
    static const std::array<QColor, 16> base = {
        QColor("#1b1f27"), QColor("#d96b73"), QColor("#72b783"), QColor("#d9ad6b"),
        QColor("#6f95d8"), QColor("#b47acb"), QColor("#65b8c5"), QColor("#d8dde6"),
        QColor("#66707d"), QColor("#ef7c85"), QColor("#83cb95"), QColor("#e8c07a"),
        QColor("#84a7e8"), QColor("#c68bdc"), QColor("#77cbd7"), QColor("#f4f6f8")
    };

    const int clamped = std::clamp(index, 0, 255);
    if (clamped < 16) {
        return base[static_cast<std::size_t>(clamped)];
    }
    if (clamped < 232) {
        const int value = clamped - 16;
        const int r = value / 36;
        const int g = (value / 6) % 6;
        const int b = value % 6;
        auto component = [](int level) { return level == 0 ? 0 : 55 + (level * 40); };
        return QColor(component(r), component(g), component(b));
    }
    const int gray = 8 + ((clamped - 232) * 10);
    return QColor(gray, gray, gray);
}

TerminalScreen::Row TerminalScreen::blankRow() const
{
    return Row(static_cast<std::size_t>(m_columns), blankCell());
}

TerminalCell TerminalScreen::blankCell() const
{
    TerminalCell cell;
    cell.text = QStringLiteral(" ");
    cell.style = m_currentStyle;
    cell.width = 1;
    cell.continuation = false;
    cell.softWrapAfter = false;
    return cell;
}

std::vector<TerminalScreen::Row>& TerminalScreen::activeRows()
{
    return m_alternateScreen ? m_alternateRows : m_primaryRows;
}

const std::vector<TerminalScreen::Row>& TerminalScreen::activeRows() const
{
    return m_alternateScreen ? m_alternateRows : m_primaryRows;
}

int TerminalScreen::codePointWidth(char32_t codePoint)
{
    if (codePoint == U'\0') {
        return 0;
    }

    // Combining marks, joiners and variation selectors extend an existing
    // grapheme and therefore do not consume their own terminal cell.
    const QChar::Category category = QChar::category(codePoint);
    if (category == QChar::Mark_NonSpacing
        || category == QChar::Mark_SpacingCombining
        || category == QChar::Mark_Enclosing
        || codePoint == U'\u200D'
        || (codePoint >= U'\uFE00' && codePoint <= U'\uFE0F')
        || (codePoint >= U'\U000E0100' && codePoint <= U'\U000E01EF')) {
        return 0;
    }

    const int width = ::wcwidth(static_cast<wchar_t>(codePoint));
    if (width == 0) {
        return 0;
    }
    if (width >= 2) {
        return 2;
    }

    // The parser filters C0/C1 controls before text reaches the screen. For
    // printable characters unknown to the current locale, one cell is the
    // safest fallback. main() enables the user's UTF-8 locale on Linux.
    return 1;
}

void TerminalScreen::appendCombiningCodePoint(char32_t codePoint)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    int column = m_cursorColumn;

    if (m_wrapPending) {
        column = m_columns - 1;
    } else if (column > 0) {
        --column;
    }

    if (column >= 0 && column < m_columns && currentRow[static_cast<std::size_t>(column)].continuation) {
        --column;
    }

    if (column < 0 || column >= m_columns) {
        return;
    }

    TerminalCell& target = currentRow[static_cast<std::size_t>(column)];
    if (target.continuation || target.text == QStringLiteral(" ")) {
        return;
    }
    target.text += QString::fromUcs4(&codePoint, 1);
}

void TerminalScreen::clearWideCellAt(Row& rowValue, int column)
{
    if (column < 0 || column >= m_columns) {
        return;
    }

    TerminalCell& current = rowValue[static_cast<std::size_t>(column)];
    if (current.continuation && column > 0) {
        rowValue[static_cast<std::size_t>(column - 1)] = blankCell();
        current = blankCell();
        return;
    }

    if (current.width == 2 && column + 1 < m_columns) {
        rowValue[static_cast<std::size_t>(column + 1)] = blankCell();
    }
    current = blankCell();
}

void TerminalScreen::sanitizeWideCells(Row& rowValue)
{
    for (int column = 0; column < static_cast<int>(rowValue.size()); ++column) {
        TerminalCell& cellValue = rowValue[static_cast<std::size_t>(column)];
        if (cellValue.continuation) {
            const bool validLead = column > 0
                && rowValue[static_cast<std::size_t>(column - 1)].width == 2
                && !rowValue[static_cast<std::size_t>(column - 1)].continuation;
            if (!validLead) {
                cellValue = blankCell();
            }
            continue;
        }

        if (cellValue.width == 2) {
            if (column + 1 >= static_cast<int>(rowValue.size())) {
                cellValue = blankCell();
                continue;
            }
            TerminalCell& continuation = rowValue[static_cast<std::size_t>(column + 1)];
            continuation.text.clear();
            continuation.style = cellValue.style;
            continuation.width = 0;
            continuation.continuation = true;
            continuation.softWrapAfter = cellValue.softWrapAfter;
            ++column;
        }
    }
}

void TerminalScreen::putCodePoint(char32_t codePoint)
{
    if (codePoint == U'\0') {
        return;
    }

    const int glyphWidth = codePointWidth(codePoint);
    if (glyphWidth == 0) {
        appendCombiningCodePoint(codePoint);
        return;
    }
    m_lastPrintedCodePoint = codePoint;

    if (m_wrapPending) {
        Row& previousRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
        if (!previousRow.empty()) {
            previousRow.back().softWrapAfter = true;
        }
        m_cursorColumn = 0;
        lineFeed();
        m_wrapPending = false;
    }

    int width = glyphWidth;
    if (width == 2 && m_columns < 2) {
        width = 1;
    }

    if (width == 2 && m_cursorColumn == m_columns - 1) {
        if (m_autoWrap) {
            Row& previousRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
            previousRow.back().softWrapAfter = true;
            m_cursorColumn = 0;
            lineFeed();
        } else {
            width = 1;
        }
    }

    if (m_insertMode) {
        insertCharacters(width);
    }

    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    clearWideCellAt(currentRow, m_cursorColumn);
    if (width == 2) {
        clearWideCellAt(currentRow, m_cursorColumn + 1);
    }

    TerminalCell& target = currentRow[static_cast<std::size_t>(m_cursorColumn)];
    target.text = QString::fromUcs4(&codePoint, 1);
    target.style = m_currentStyle;
    target.width = width;
    target.continuation = false;
    target.softWrapAfter = false;

    if (width == 2) {
        TerminalCell& continuation = currentRow[static_cast<std::size_t>(m_cursorColumn + 1)];
        continuation.text.clear();
        continuation.style = m_currentStyle;
        continuation.width = 0;
        continuation.continuation = true;
        continuation.softWrapAfter = false;
    }

    const int lastOccupiedColumn = m_cursorColumn + width - 1;
    if (lastOccupiedColumn >= m_columns - 1) {
        m_cursorColumn = m_columns - 1;
        m_wrapPending = m_autoWrap;
    } else {
        m_cursorColumn += width;
    }
}

void TerminalScreen::scrollRegionUp(int count, bool collectScrollback)
{
    auto& rowsBuffer = activeRows();
    const int amount = std::min(clampPositiveCount(count), m_scrollBottom - m_scrollTop + 1);
    for (int i = 0; i < amount; ++i) {
        if (collectScrollback) {
            m_scrollback.push_back(rowsBuffer[static_cast<std::size_t>(m_scrollTop)]);
            if (m_scrollback.size() > MaxScrollbackRows) {
                m_scrollback.pop_front();
            }
        }
        rowsBuffer.erase(rowsBuffer.begin() + m_scrollTop);
        rowsBuffer.insert(rowsBuffer.begin() + m_scrollBottom, blankRow());
    }
}

void TerminalScreen::scrollRegionDown(int count)
{
    auto& rowsBuffer = activeRows();
    const int amount = std::min(clampPositiveCount(count), m_scrollBottom - m_scrollTop + 1);
    for (int i = 0; i < amount; ++i) {
        rowsBuffer.erase(rowsBuffer.begin() + m_scrollBottom);
        rowsBuffer.insert(rowsBuffer.begin() + m_scrollTop, blankRow());
    }
}

void TerminalScreen::clampCursor()
{
    m_cursorRow = std::clamp(m_cursorRow, 0, m_rows - 1);
    m_cursorColumn = std::clamp(m_cursorColumn, 0, m_columns - 1);
    m_savedCursorRow = std::clamp(m_savedCursorRow, 0, m_rows - 1);
    m_savedCursorColumn = std::clamp(m_savedCursorColumn, 0, m_columns - 1);
}

void TerminalScreen::resetMargins()
{
    m_scrollTop = 0;
    m_scrollBottom = m_rows - 1;
}

void TerminalScreen::resetTabStops()
{
    m_tabStops.assign(static_cast<std::size_t>(m_columns), false);
    for (int column = 8; column < m_columns; column += 8) {
        m_tabStops[static_cast<std::size_t>(column)] = true;
    }
}
