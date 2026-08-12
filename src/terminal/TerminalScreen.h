#pragma once

#include <QColor>
#include <QString>

#include <deque>
#include <vector>

struct TerminalCellStyle
{
    QColor foreground{QColor(QStringLiteral("#e9edf2"))};
    QColor background{QColor(QStringLiteral("#0d0f12"))};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool inverse{false};

    friend bool operator==(const TerminalCellStyle&, const TerminalCellStyle&) = default;
};

struct TerminalCell
{
    QString text{QStringLiteral(" ")};
    TerminalCellStyle style{};
};

class TerminalScreen final
{
public:
    using Row = std::vector<TerminalCell>;

    TerminalScreen(int rows = 30, int columns = 120);

    [[nodiscard]] int rows() const noexcept;
    [[nodiscard]] int columns() const noexcept;
    [[nodiscard]] int cursorRow() const noexcept;
    [[nodiscard]] int cursorColumn() const noexcept;
    [[nodiscard]] bool cursorVisible() const noexcept;
    [[nodiscard]] bool bracketedPaste() const noexcept;
    [[nodiscard]] bool applicationCursorKeys() const noexcept;
    [[nodiscard]] bool alternateScreenActive() const noexcept;
    [[nodiscard]] int scrollbackRows() const noexcept;
    [[nodiscard]] int historyRows() const noexcept;
    [[nodiscard]] const Row& row(int index) const;
    [[nodiscard]] const Row& historyRow(int index) const;
    [[nodiscard]] const TerminalCell& cell(int row, int column) const;
    [[nodiscard]] QString textInRange(int startRow, int startColumn, int endRow, int endColumn) const;
    [[nodiscard]] QString textInHistoryRange(int startRow, int startColumn, int endRow, int endColumn) const;

    void reset();
    void resize(int rows, int columns);

    void writeText(const QString& text);
    void carriageReturn();
    void lineFeed();
    void backspace();
    void horizontalTab();

    void cursorUp(int count = 1);
    void cursorDown(int count = 1);
    void cursorForward(int count = 1);
    void cursorBackward(int count = 1);
    void cursorNextLine(int count = 1);
    void cursorPreviousLine(int count = 1);
    void setCursorPosition(int row, int column);
    void setCursorRow(int row);
    void setCursorColumn(int column);
    void saveCursor();
    void restoreCursor();

    void eraseDisplay(int mode);
    void eraseLine(int mode);
    void eraseCharacters(int count);
    void insertCharacters(int count);
    void deleteCharacters(int count);
    void insertLines(int count);
    void deleteLines(int count);
    void scrollUp(int count = 1);
    void scrollDown(int count = 1);
    void setScrollRegion(int top, int bottom);

    void resetStyle();
    void setBold(bool enabled);
    void setItalic(bool enabled);
    void setUnderline(bool enabled);
    void setInverse(bool enabled);
    void setForeground(const QColor& color);
    void setBackground(const QColor& color);
    void setDefaultForeground();
    void setDefaultBackground();

    void setCursorVisible(bool visible);
    void setBracketedPaste(bool enabled);
    void setApplicationCursorKeys(bool enabled);
    void setAutoWrap(bool enabled);
    void useAlternateScreen(bool enabled, bool clearOnEnter = true);

    [[nodiscard]] static QColor defaultForeground();
    [[nodiscard]] static QColor defaultBackground();
    [[nodiscard]] static QColor indexedColor(int index);

private:
    [[nodiscard]] Row blankRow() const;
    [[nodiscard]] TerminalCell blankCell() const;
    [[nodiscard]] std::vector<Row>& activeRows();
    [[nodiscard]] const std::vector<Row>& activeRows() const;
    void putCodePoint(char32_t codePoint);
    void scrollRegionUp(int count, bool collectScrollback);
    void scrollRegionDown(int count);
    void clampCursor();
    void resetMargins();

    int m_rows;
    int m_columns;
    int m_cursorRow{0};
    int m_cursorColumn{0};
    int m_savedCursorRow{0};
    int m_savedCursorColumn{0};
    int m_scrollTop{0};
    int m_scrollBottom{0};
    bool m_cursorVisible{true};
    bool m_bracketedPaste{false};
    bool m_applicationCursorKeys{false};
    bool m_autoWrap{true};
    bool m_wrapPending{false};
    bool m_alternateScreen{false};

    int m_primaryCursorRow{0};
    int m_primaryCursorColumn{0};

    TerminalCellStyle m_currentStyle{};
    std::vector<Row> m_primaryRows;
    std::vector<Row> m_alternateRows;
    std::deque<Row> m_scrollback;

    static constexpr std::size_t MaxScrollbackRows = 5000;
};
