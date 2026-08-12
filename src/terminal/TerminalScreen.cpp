#include "TerminalScreen.h"

#include <QChar>

#include <algorithm>
#include <array>

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
}

int TerminalScreen::rows() const noexcept { return m_rows; }
int TerminalScreen::columns() const noexcept { return m_columns; }
int TerminalScreen::cursorRow() const noexcept { return m_cursorRow; }
int TerminalScreen::cursorColumn() const noexcept { return m_cursorColumn; }
bool TerminalScreen::cursorVisible() const noexcept { return m_cursorVisible; }
bool TerminalScreen::bracketedPaste() const noexcept { return m_bracketedPaste; }
bool TerminalScreen::applicationCursorKeys() const noexcept { return m_applicationCursorKeys; }
bool TerminalScreen::alternateScreenActive() const noexcept { return m_alternateScreen; }
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

        QString line;
        for (int columnIndex = firstColumn; columnIndex <= lastColumn; ++columnIndex) {
            const QString& text = cell(rowIndex, columnIndex).text;
            line += text.isEmpty() ? QStringLiteral(" ") : text;
        }

        while (line.endsWith(QLatin1Char(' '))) {
            line.chop(1);
        }

        result += line;
        if (rowIndex != endRow) {
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
            const QString& text = sourceRow.at(static_cast<std::size_t>(columnIndex)).text;
            line += text.isEmpty() ? QStringLiteral(" ") : text;
        }

        while (line.endsWith(QLatin1Char(' '))) {
            line.chop(1);
        }

        result += line;
        if (rowIndex != endRow) {
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
    m_bracketedPaste = false;
    m_applicationCursorKeys = false;
    m_autoWrap = true;
    m_wrapPending = false;
    m_alternateScreen = false;
    m_currentStyle = TerminalCellStyle{};
    resetMargins();
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

    auto resizeBuffer = [&](std::vector<Row>& buffer) {
        for (Row& existingRow : buffer) {
            existingRow.resize(static_cast<std::size_t>(newColumns), blankCell());
        }
        if (static_cast<int>(buffer.size()) < newRows) {
            buffer.reserve(static_cast<std::size_t>(newRows));
            while (static_cast<int>(buffer.size()) < newRows) {
                Row newRow(static_cast<std::size_t>(newColumns), blankCell());
                buffer.push_back(std::move(newRow));
            }
        } else if (static_cast<int>(buffer.size()) > newRows) {
            buffer.resize(static_cast<std::size_t>(newRows));
        }
    };

    m_rows = newRows;
    m_columns = newColumns;
    resizeBuffer(m_primaryRows);
    resizeBuffer(m_alternateRows);
    for (Row& historyRowValue : m_scrollback) {
        historyRowValue.resize(static_cast<std::size_t>(newColumns), blankCell());
    }
    resetMargins();
    clampCursor();
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
    m_wrapPending = false;
    const int nextStop = ((m_cursorColumn / 8) + 1) * 8;
    m_cursorColumn = std::min(m_columns - 1, nextStop);
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
}

void TerminalScreen::restoreCursor()
{
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
}

void TerminalScreen::eraseCharacters(int count)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    const int end = std::min(m_columns, m_cursorColumn + clampPositiveCount(count));
    std::fill(currentRow.begin() + m_cursorColumn, currentRow.begin() + end, blankCell());
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
}

void TerminalScreen::deleteCharacters(int count)
{
    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    const int amount = std::min(clampPositiveCount(count), m_columns - m_cursorColumn);
    std::move(currentRow.begin() + m_cursorColumn + amount,
              currentRow.end(),
              currentRow.begin() + m_cursorColumn);
    std::fill(currentRow.end() - amount, currentRow.end(), blankCell());
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
    setCursorPosition(0, 0);
}

void TerminalScreen::resetStyle() { m_currentStyle = TerminalCellStyle{}; }
void TerminalScreen::setBold(bool enabled) { m_currentStyle.bold = enabled; }
void TerminalScreen::setItalic(bool enabled) { m_currentStyle.italic = enabled; }
void TerminalScreen::setUnderline(bool enabled) { m_currentStyle.underline = enabled; }
void TerminalScreen::setInverse(bool enabled) { m_currentStyle.inverse = enabled; }
void TerminalScreen::setForeground(const QColor& color) { m_currentStyle.foreground = color; }
void TerminalScreen::setBackground(const QColor& color) { m_currentStyle.background = color; }
void TerminalScreen::setDefaultForeground() { m_currentStyle.foreground = defaultForeground(); }
void TerminalScreen::setDefaultBackground() { m_currentStyle.background = defaultBackground(); }
void TerminalScreen::setCursorVisible(bool visible) { m_cursorVisible = visible; }
void TerminalScreen::setBracketedPaste(bool enabled) { m_bracketedPaste = enabled; }
void TerminalScreen::setApplicationCursorKeys(bool enabled) { m_applicationCursorKeys = enabled; }
void TerminalScreen::setAutoWrap(bool enabled) { m_autoWrap = enabled; }

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

QColor TerminalScreen::defaultForeground() { return QColor(QStringLiteral("#e9edf2")); }
QColor TerminalScreen::defaultBackground() { return QColor(QStringLiteral("#0d0f12")); }

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

void TerminalScreen::putCodePoint(char32_t codePoint)
{
    if (codePoint == U'\0') {
        return;
    }

    if (m_wrapPending) {
        m_cursorColumn = 0;
        lineFeed();
        m_wrapPending = false;
    }

    Row& currentRow = activeRows()[static_cast<std::size_t>(m_cursorRow)];
    TerminalCell& target = currentRow[static_cast<std::size_t>(m_cursorColumn)];
    target.text = QString::fromUcs4(&codePoint, 1);
    target.style = m_currentStyle;

    if (m_cursorColumn == m_columns - 1) {
        m_wrapPending = m_autoWrap;
    } else {
        ++m_cursorColumn;
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
