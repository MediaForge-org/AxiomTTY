#pragma once

#include <QColor>
#include <QString>

#include <deque>
#include <vector>

enum class TerminalCursorShape
{
    Block,
    Underline,
    Bar
};

enum class TerminalMouseTrackingMode
{
    None,
    Normal,
    ButtonEvent,
    AnyEvent
};

struct TerminalCellStyle
{
    QColor foreground{QColor(QStringLiteral("#e9edf2"))};
    QColor background{QColor(QStringLiteral("#0d0f12"))};
    bool bold{false};
    bool italic{false};
    bool underline{false};
    bool faint{false};
    bool strikethrough{false};
    bool inverse{false};

    friend bool operator==(const TerminalCellStyle&, const TerminalCellStyle&) = default;
};

struct TerminalCell
{
    QString text{QStringLiteral(" ")};
    TerminalCellStyle style{};
    int width{1};
    bool continuation{false};
    bool softWrapAfter{false};
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
    [[nodiscard]] TerminalCursorShape cursorShape() const noexcept;
    [[nodiscard]] bool cursorBlinking() const noexcept;
    [[nodiscard]] bool bracketedPaste() const noexcept;
    [[nodiscard]] bool applicationCursorKeys() const noexcept;
    [[nodiscard]] bool alternateScreenActive() const noexcept;
    [[nodiscard]] bool originMode() const noexcept;
    [[nodiscard]] bool insertMode() const noexcept;
    [[nodiscard]] TerminalMouseTrackingMode mouseTrackingMode() const noexcept;
    [[nodiscard]] bool sgrMouseMode() const noexcept;
    [[nodiscard]] bool focusReporting() const noexcept;
    [[nodiscard]] bool alternateScroll() const noexcept;
    [[nodiscard]] bool synchronizedOutput() const noexcept;
    [[nodiscard]] int scrollTop() const noexcept;
    [[nodiscard]] int scrollBottom() const noexcept;
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
    void cursorForwardTab(int count = 1);
    void cursorBackwardTab(int count = 1);
    void setTabStopAtCursor();
    void clearTabStopAtCursor();
    void clearAllTabStops();
    void repeatLastCharacter(int count = 1);

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
    void setFaint(bool enabled);
    void setStrikethrough(bool enabled);
    void setInverse(bool enabled);
    void setForeground(const QColor& color);
    void setBackground(const QColor& color);
    void setDefaultForeground();
    void setDefaultBackground();

    void setCursorVisible(bool visible);
    void setCursorStyle(int styleCode);
    void setCursorBlinking(bool enabled);
    void setBracketedPaste(bool enabled);
    void setApplicationCursorKeys(bool enabled);
    void setAutoWrap(bool enabled);
    void setOriginMode(bool enabled);
    void setInsertMode(bool enabled);
    void setMouseTrackingMode(TerminalMouseTrackingMode mode);
    void setSgrMouseMode(bool enabled);
    void setFocusReporting(bool enabled);
    void setAlternateScroll(bool enabled);
    void setSynchronizedOutput(bool enabled);
    void useAlternateScreen(bool enabled, bool clearOnEnter = true);

    [[nodiscard]] static QColor defaultForeground();
    [[nodiscard]] static QColor defaultBackground();
    [[nodiscard]] static QColor indexedColor(int index);

private:
    [[nodiscard]] Row blankRow() const;
    [[nodiscard]] TerminalCell blankCell() const;
    [[nodiscard]] std::vector<Row>& activeRows();
    [[nodiscard]] const std::vector<Row>& activeRows() const;
    [[nodiscard]] static int codePointWidth(char32_t codePoint);
    void putCodePoint(char32_t codePoint);
    void appendCombiningCodePoint(char32_t codePoint);
    void clearWideCellAt(Row& row, int column);
    void sanitizeWideCells(Row& row);
    void scrollRegionUp(int count, bool collectScrollback);
    void scrollRegionDown(int count);
    void clampCursor();
    void resetMargins();
    void resetTabStops();

    int m_rows;
    int m_columns;
    int m_cursorRow{0};
    int m_cursorColumn{0};
    int m_savedCursorRow{0};
    int m_savedCursorColumn{0};
    int m_scrollTop{0};
    int m_scrollBottom{0};
    bool m_cursorVisible{true};
    TerminalCursorShape m_cursorShape{TerminalCursorShape::Block};
    bool m_cursorBlinking{true};
    bool m_bracketedPaste{false};
    bool m_applicationCursorKeys{false};
    bool m_autoWrap{true};
    bool m_originMode{false};
    bool m_insertMode{false};
    TerminalMouseTrackingMode m_mouseTrackingMode{TerminalMouseTrackingMode::None};
    bool m_sgrMouseMode{false};
    bool m_focusReporting{false};
    bool m_alternateScroll{false};
    bool m_synchronizedOutput{false};
    bool m_wrapPending{false};
    bool m_alternateScreen{false};

    int m_primaryCursorRow{0};
    int m_primaryCursorColumn{0};

    TerminalCellStyle m_currentStyle{};
    TerminalCellStyle m_savedStyle{};
    bool m_savedAutoWrap{true};
    bool m_savedOriginMode{false};
    char32_t m_lastPrintedCodePoint{U'\0'};
    std::vector<bool> m_tabStops;
    std::vector<Row> m_primaryRows;
    std::vector<Row> m_alternateRows;
    std::deque<Row> m_scrollback;

    static constexpr std::size_t MaxScrollbackRows = 5000;
};
