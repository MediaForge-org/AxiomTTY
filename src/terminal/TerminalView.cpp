#include "TerminalView.h"

#include "TerminalScreen.h"
#include "TerminalSession.h"

#include <QClipboard>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

TerminalView::TerminalView(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_fontFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family())
{
    setAntialiasing(false);
    setFillColor(QColor());
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    m_cursorBlinkTimer.setInterval(550);
    m_cursorBlinkTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_cursorBlinkTimer, &QTimer::timeout, this, [this] {
        m_cursorBlinkOn = !m_cursorBlinkOn;
        update();
    });
    m_cursorBlinkTimer.start();

    // While extending a selection, keep scrolling through history when the
    // pointer reaches or leaves the top/bottom edge of the terminal viewport.
    // This makes selection operate on the whole scrollback buffer instead of
    // being limited to the currently visible rows.
    m_selectionAutoScrollTimer.setInterval(38);
    m_selectionAutoScrollTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_selectionAutoScrollTimer, &QTimer::timeout,
            this, &TerminalView::autoScrollSelection);

    updateMetrics();
}

QObject* TerminalView::session() const
{
    return m_session.data();
}

void TerminalView::setSession(QObject* sessionObject)
{
    auto* newSession = qobject_cast<TerminalSession*>(sessionObject);
    if (m_session == newSession) {
        return;
    }

    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
    }

    clearSelection();
    if (m_scrollbackOffset != 0) {
        m_scrollbackOffset = 0;
        emit scrollbackChanged();
    }
    m_session = newSession;
    if (m_session) {
        connect(m_session, &TerminalSession::screenChanged, this, [this] {
            clampScrollbackOffset();
            if (m_searchActive && !m_searchQuery.isEmpty()) {
                rebuildSearchMatches(true);
            }
            update();
        });
        connect(m_session, &TerminalSession::runningChanged, this, [this] { update(); });
    }

    emit sessionChanged();
    updateTerminalSize();
    update();
}

QString TerminalView::fontFamily() const { return m_fontFamily; }

void TerminalView::setFontFamily(const QString& family)
{
    if (family.isEmpty() || m_fontFamily == family) {
        return;
    }
    m_fontFamily = family;
    emit fontFamilyChanged();
    updateMetrics();
}

qreal TerminalView::fontPixelSize() const noexcept { return m_fontPixelSize; }

void TerminalView::setFontPixelSize(qreal size)
{
    const qreal clamped = std::clamp(size, 8.0, 48.0);
    if (qFuzzyCompare(m_fontPixelSize, clamped)) {
        return;
    }
    m_fontPixelSize = clamped;
    emit fontPixelSizeChanged();
    updateMetrics();
}

qreal TerminalView::cellWidth() const noexcept { return m_cellWidth; }
qreal TerminalView::cellHeight() const noexcept { return m_cellHeight; }
bool TerminalView::hasSelection() const noexcept { return m_selectionActive; }

QString TerminalView::selectedText() const
{
    if (!m_session || !hasSelection()) {
        return {};
    }

    const auto [start, end] = normalizedSelection();
    return m_session->screen().textInHistoryRange(
        start.y(), start.x(), end.y(), end.x());
}

int TerminalView::scrollbackOffset() const noexcept { return m_scrollbackOffset; }
bool TerminalView::searchActive() const noexcept { return m_searchActive; }
QString TerminalView::searchQuery() const { return m_searchQuery; }

void TerminalView::setSearchQuery(const QString& query)
{
    if (m_searchQuery == query) {
        return;
    }
    m_searchQuery = query;
    rebuildSearchMatches(false);
}

bool TerminalView::searchCaseSensitive() const noexcept { return m_searchCaseSensitive; }

void TerminalView::setSearchCaseSensitive(bool enabled)
{
    if (m_searchCaseSensitive == enabled) {
        return;
    }
    m_searchCaseSensitive = enabled;
    rebuildSearchMatches(false);
}

int TerminalView::searchMatchCount() const noexcept
{
    return static_cast<int>(m_searchMatches.size());
}

int TerminalView::currentSearchMatch() const noexcept
{
    return m_currentSearchIndex >= 0 ? m_currentSearchIndex + 1 : 0;
}

bool TerminalView::copySelection()
{
    const QString text = selectedText();
    if (text.isEmpty() && !hasSelection()) {
        return false;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        return false;
    }

    clipboard->setText(text, QClipboard::Clipboard);
    emit selectionCopied();
    return true;
}

void TerminalView::clearSelection()
{
    stopSelectionAutoScroll();
    if (!m_selectionActive && !m_selecting) {
        return;
    }

    m_selectionActive = false;
    m_selecting = false;
    emit selectionChanged();
    update();
}

void TerminalView::scrollPageUp()
{
    if (!m_session || m_session->screen().alternateScreenActive()) {
        return;
    }
    scrollByRows(std::max(1, m_session->screen().rows() - 2));
}

void TerminalView::scrollPageDown()
{
    if (!m_session || m_session->screen().alternateScreenActive()) {
        return;
    }
    scrollByRows(-std::max(1, m_session->screen().rows() - 2));
}

void TerminalView::scrollToBottom()
{
    if (m_scrollbackOffset == 0) {
        return;
    }
    clearSelection();
    m_scrollbackOffset = 0;
    emit scrollbackChanged();
    update();
}

void TerminalView::beginSearch()
{
    if (!m_searchActive) {
        m_searchActive = true;
        rebuildSearchMatches(false);
    }
    emit searchRequested();
    emit searchChanged();
    update();
}

void TerminalView::endSearch()
{
    if (!m_searchActive && m_searchQuery.isEmpty() && m_searchMatches.empty()) {
        return;
    }

    m_searchActive = false;
    m_searchQuery.clear();
    m_searchMatches.clear();
    m_currentSearchIndex = -1;
    emit searchChanged();
    update();
}

void TerminalView::findNext()
{
    if (m_searchMatches.empty()) {
        return;
    }

    const int count = static_cast<int>(m_searchMatches.size());
    const int next = m_currentSearchIndex < 0 ? 0 : (m_currentSearchIndex + 1) % count;
    activateSearchMatch(next);
}

void TerminalView::findPrevious()
{
    if (m_searchMatches.empty()) {
        return;
    }

    const int count = static_cast<int>(m_searchMatches.size());
    const int previous = m_currentSearchIndex < 0
        ? count - 1
        : (m_currentSearchIndex - 1 + count) % count;
    activateSearchMatch(previous);
}

void TerminalView::paint(QPainter* painter)
{
    painter->fillRect(boundingRect(), TerminalScreen::defaultBackground());
    if (!m_session) {
        return;
    }

    const TerminalScreen& screen = m_session->screen();
    QFont baseFont(m_fontFamily);
    baseFont.setPixelSize(static_cast<int>(std::round(m_fontPixelSize)));
    baseFont.setStyleHint(QFont::Monospace);

    painter->setRenderHint(QPainter::TextAntialiasing, true);

    const QColor selectionBackground(QStringLiteral("#2d4767"));
    const QColor selectionForeground(QStringLiteral("#f4f7fb"));
    const QColor searchBackground(QStringLiteral("#39424f"));
    const QColor searchCurrentBackground(QStringLiteral("#756129"));
    const QColor searchForeground(QStringLiteral("#f4f7fb"));

    const int historyStart = visibleHistoryStart();
    for (int rowIndex = 0; rowIndex < screen.rows(); ++rowIndex) {
        const TerminalScreen::Row& row = screen.historyRow(historyStart + rowIndex);
        const qreal top = static_cast<qreal>(rowIndex) * m_cellHeight;
        if (top >= height()) {
            break;
        }

        for (int column = 0; column < screen.columns(); ++column) {
            const TerminalCell& cell = row.at(static_cast<std::size_t>(column));
            const qreal left = static_cast<qreal>(column) * m_cellWidth;
            if (left >= width()) {
                break;
            }

            QColor foreground = cell.style.foreground;
            QColor background = cell.style.background;
            if (cell.style.inverse) {
                std::swap(foreground, background);
            }
            if (cell.style.faint) {
                foreground.setAlphaF(foreground.alphaF() * 0.58);
            }

            const int searchHighlight = searchHighlightAt(historyStart + rowIndex, column);
            if (searchHighlight > 0) {
                background = searchHighlight == 2 ? searchCurrentBackground : searchBackground;
                foreground = searchForeground;
            }

            const bool selected = isCellSelected(rowIndex, column);
            if (selected) {
                background = selectionBackground;
                foreground = selectionForeground;
            }

            const QRectF cellRect(left, top, m_cellWidth + 0.5, m_cellHeight + 0.5);
            if (selected || background != TerminalScreen::defaultBackground()) {
                painter->fillRect(cellRect, background);
            }

            if (!cell.continuation && !cell.text.isEmpty() && cell.text != QStringLiteral(" ")) {
                QFont font = baseFont;
                font.setBold(cell.style.bold);
                font.setItalic(cell.style.italic);
                font.setUnderline(cell.style.underline);
                font.setStrikeOut(cell.style.strikethrough);
                painter->setFont(font);
                painter->setPen(foreground);
                painter->drawText(QPointF(left, top + m_ascent), cell.text);
            }
        }
    }

    const bool showCursor = m_scrollbackOffset == 0
        && screen.cursorVisible()
        && m_session->running()
        && (!screen.cursorBlinking() || m_cursorBlinkOn || !hasActiveFocus());

    if (showCursor) {
        const qreal left = static_cast<qreal>(screen.cursorColumn()) * m_cellWidth;
        const qreal top = static_cast<qreal>(screen.cursorRow()) * m_cellHeight;
        const QColor cursorColor(QStringLiteral("#86a9ff"));

        switch (screen.cursorShape()) {
        case TerminalCursorShape::Block: {
            const QRectF cursorRect(left, top, m_cellWidth, m_cellHeight);
            painter->fillRect(cursorRect, cursorColor);
            const TerminalCell& cursorCell = screen.cell(screen.cursorRow(), screen.cursorColumn());
            if (!cursorCell.continuation && !cursorCell.text.isEmpty() && cursorCell.text != QStringLiteral(" ")) {
                QFont font = baseFont;
                font.setBold(cursorCell.style.bold);
                font.setItalic(cursorCell.style.italic);
                font.setUnderline(cursorCell.style.underline);
                font.setStrikeOut(cursorCell.style.strikethrough);
                painter->setFont(font);
                painter->setPen(TerminalScreen::defaultBackground());
                painter->drawText(QPointF(left, top + m_ascent), cursorCell.text);
            }
            break;
        }
        case TerminalCursorShape::Underline:
            painter->fillRect(QRectF(left, top + m_cellHeight - 2.0, m_cellWidth, 2.0), cursorColor);
            break;
        case TerminalCursorShape::Bar:
            painter->fillRect(QRectF(left, top, 2.0, m_cellHeight), cursorColor);
            break;
        }
    }
}

void TerminalView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        clearSelection();
        updateTerminalSize();
        clampScrollbackOffset();
    }
}

void TerminalView::keyPressEvent(QKeyEvent* event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    const Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool ctrl = modifiers.testFlag(Qt::ControlModifier);
    const bool shift = modifiers.testFlag(Qt::ShiftModifier);
    const bool alt = modifiers.testFlag(Qt::AltModifier);

    // Search and copying remain useful even if the foreground shell/process
    // has already exited and the pane is showing historical output.
    if (ctrl && !alt && event->key() == Qt::Key_F) {
        beginSearch();
        event->accept();
        return;
    }

    if (m_searchActive && !ctrl && !alt && event->key() == Qt::Key_F3) {
        if (shift) {
            findPrevious();
        } else {
            findNext();
        }
        event->accept();
        return;
    }

    // Standard desktop copy, without sacrificing Unix ^C:
    // active selection => copy; otherwise Ctrl+C interrupts the foreground job.
    if (ctrl && !alt && event->key() == Qt::Key_C) {
        if (hasSelection()) {
            copySelection();
        } else if (!shift && m_session->running()) {
            m_session->sendInterrupt();
        }
        event->accept();
        return;
    }

    if (!m_session->running()) {
        event->ignore();
        return;
    }

    wakeCursor();

    // Ctrl+V is the normal paste shortcut. Ctrl+Shift+V remains an alias.
    if (ctrl && !alt && event->key() == Qt::Key_V) {
        clearSelection();
        scrollToBottom();
        m_session->pasteClipboard();
        event->accept();
        return;
    }

    if (shift && !ctrl && !alt && event->key() == Qt::Key_PageUp) {
        scrollPageUp();
        event->accept();
        return;
    }
    if (shift && !ctrl && !alt && event->key() == Qt::Key_PageDown) {
        scrollPageDown();
        event->accept();
        return;
    }

    if (ctrl && !alt && event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z) {
        const ushort controlCode = static_cast<ushort>(event->key() - Qt::Key_A + 1);
        sendSequence(QString(1, QChar(controlCode)));
        event->accept();
        return;
    }

    const int modifierParameter = 1
        + (shift ? 1 : 0)
        + (alt ? 2 : 0)
        + (ctrl ? 4 : 0);

    const auto cursorSequence = [this, modifierParameter](char finalByte, const char* normal, const char* application) {
        if (modifierParameter != 1) {
            return QStringLiteral("\x1b[1;%1%2")
                .arg(modifierParameter)
                .arg(QChar::fromLatin1(finalByte));
        }
        return QString::fromLatin1(m_session->applicationCursorKeys() ? application : normal);
    };

    const auto tildeSequence = [modifierParameter](int number) {
        if (modifierParameter == 1) {
            return QStringLiteral("\x1b[%1~").arg(number);
        }
        return QStringLiteral("\x1b[%1;%2~").arg(number).arg(modifierParameter);
    };

    const auto functionSequence = [modifierParameter](char finalByte, const char* plain) {
        if (modifierParameter == 1) {
            return QString::fromLatin1(plain);
        }
        return QStringLiteral("\x1b[1;%1%2")
            .arg(modifierParameter)
            .arg(QChar::fromLatin1(finalByte));
    };

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: sendSequence(QStringLiteral("\r")); break;
    case Qt::Key_Backspace: sendSequence(QString(1, QChar(0x7f))); break;
    case Qt::Key_Tab: sendSequence(shift ? QStringLiteral("\x1b[Z") : QStringLiteral("\t")); break;
    case Qt::Key_Escape: sendSequence(QStringLiteral("\x1b")); break;
    case Qt::Key_Up: sendSequence(cursorSequence('A', "\x1b[A", "\x1bOA")); break;
    case Qt::Key_Down: sendSequence(cursorSequence('B', "\x1b[B", "\x1bOB")); break;
    case Qt::Key_Right: sendSequence(cursorSequence('C', "\x1b[C", "\x1bOC")); break;
    case Qt::Key_Left: sendSequence(cursorSequence('D', "\x1b[D", "\x1bOD")); break;
    case Qt::Key_Home: sendSequence(cursorSequence('H', "\x1b[H", "\x1bOH")); break;
    case Qt::Key_End: sendSequence(cursorSequence('F', "\x1b[F", "\x1bOF")); break;
    case Qt::Key_Insert: sendSequence(tildeSequence(2)); break;
    case Qt::Key_Delete: sendSequence(tildeSequence(3)); break;
    case Qt::Key_PageUp: sendSequence(tildeSequence(5)); break;
    case Qt::Key_PageDown: sendSequence(tildeSequence(6)); break;
    case Qt::Key_F1: sendSequence(functionSequence('P', "\x1bOP")); break;
    case Qt::Key_F2: sendSequence(functionSequence('Q', "\x1bOQ")); break;
    case Qt::Key_F3: sendSequence(functionSequence('R', "\x1bOR")); break;
    case Qt::Key_F4: sendSequence(functionSequence('S', "\x1bOS")); break;
    case Qt::Key_F5: sendSequence(tildeSequence(15)); break;
    case Qt::Key_F6: sendSequence(tildeSequence(17)); break;
    case Qt::Key_F7: sendSequence(tildeSequence(18)); break;
    case Qt::Key_F8: sendSequence(tildeSequence(19)); break;
    case Qt::Key_F9: sendSequence(tildeSequence(20)); break;
    case Qt::Key_F10: sendSequence(tildeSequence(21)); break;
    case Qt::Key_F11: sendSequence(tildeSequence(23)); break;
    case Qt::Key_F12: sendSequence(tildeSequence(24)); break;
    default: {
        if (event->text().isEmpty() || ctrl) {
            event->ignore();
            return;
        }
        QString text = event->text();
        if (alt) {
            text.prepend(QChar(0x1b));
        }
        sendSequence(text);
        break;
    }
    }

    event->accept();
}

void TerminalView::focusInEvent(QFocusEvent* event)
{
    QQuickPaintedItem::focusInEvent(event);
    wakeCursor();
    if (m_session && m_session->running() && m_session->screen().focusReporting()) {
        m_session->sendText(QStringLiteral("\x1b[I"));
    }
}

void TerminalView::focusOutEvent(QFocusEvent* event)
{
    QQuickPaintedItem::focusOutEvent(event);
    if (m_session && m_session->running() && m_session->screen().focusReporting()) {
        m_session->sendText(QStringLiteral("\x1b[O"));
    }
    update();
}

void TerminalView::mousePressEvent(QMouseEvent* event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    forceActiveFocus();
    wakeCursor();

    if (terminalOwnsMouse(event->modifiers())) {
        int buttonCode = -1;
        if (event->button() == Qt::LeftButton) buttonCode = 0;
        else if (event->button() == Qt::MiddleButton) buttonCode = 1;
        else if (event->button() == Qt::RightButton) buttonCode = 2;
        if (buttonCode >= 0) {
            sendMouseReport(buttonCode, event->position(), false, false);
            event->accept();
            return;
        }
    }

    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    stopSelectionAutoScroll();
    m_lastSelectionPointer = event->position();
    m_selectionStart = cellAt(event->position());
    m_selectionEnd = m_selectionStart;
    m_selecting = true;
    m_selectionActive = false;
    emit selectionChanged();
    update();
    event->accept();
}

void TerminalView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    if (terminalOwnsMouse(event->modifiers())) {
        const TerminalMouseTrackingMode mode = m_session->screen().mouseTrackingMode();
        if (mode == TerminalMouseTrackingMode::ButtonEvent || mode == TerminalMouseTrackingMode::AnyEvent) {
            int buttonCode = 3;
            if (event->buttons().testFlag(Qt::LeftButton)) buttonCode = 0;
            else if (event->buttons().testFlag(Qt::MiddleButton)) buttonCode = 1;
            else if (event->buttons().testFlag(Qt::RightButton)) buttonCode = 2;

            if (mode == TerminalMouseTrackingMode::AnyEvent || buttonCode != 3) {
                sendMouseReport(buttonCode, event->position(), false, true);
                event->accept();
                return;
            }
        }
    }

    if (!m_selecting || !(event->buttons() & Qt::LeftButton)) {
        event->ignore();
        return;
    }

    m_lastSelectionPointer = event->position();
    setSelectionEnd(cellAt(event->position()));
    updateSelectionAutoScroll(event->position());
    event->accept();
}

void TerminalView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    if (terminalOwnsMouse(event->modifiers())) {
        int buttonCode = -1;
        if (event->button() == Qt::LeftButton) buttonCode = 0;
        else if (event->button() == Qt::MiddleButton) buttonCode = 1;
        else if (event->button() == Qt::RightButton) buttonCode = 2;
        if (buttonCode >= 0) {
            sendMouseReport(buttonCode, event->position(), true, false);
            event->accept();
            return;
        }
    }

    if (event->button() != Qt::LeftButton || !m_selecting) {
        event->ignore();
        return;
    }

    m_lastSelectionPointer = event->position();
    stopSelectionAutoScroll();
    setSelectionEnd(cellAt(event->position()));
    m_selecting = false;
    m_selectionActive = m_selectionStart != m_selectionEnd;
    emit selectionChanged();
    update();
    event->accept();
}

void TerminalView::wheelEvent(QWheelEvent* event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    int steps = event->angleDelta().y() / 120;
    if (steps == 0 && event->pixelDelta().y() != 0) {
        steps = event->pixelDelta().y() > 0 ? 1 : -1;
    }
    if (steps == 0) {
        event->ignore();
        return;
    }

    if (terminalOwnsMouse(event->modifiers())) {
        const int buttonCode = steps > 0 ? 64 : 65;
        for (int index = 0; index < std::abs(steps); ++index) {
            sendMouseReport(buttonCode, event->position(), false, false);
        }
        event->accept();
        return;
    }

    if (m_session->screen().alternateScreenActive()) {
        if (m_session->screen().alternateScroll()) {
            const QString sequence = steps > 0
                ? QString::fromLatin1(m_session->applicationCursorKeys() ? "\x1bOA" : "\x1b[A")
                : QString::fromLatin1(m_session->applicationCursorKeys() ? "\x1bOB" : "\x1b[B");
            for (int index = 0; index < std::abs(steps) * 3; ++index) {
                m_session->sendText(sequence);
            }
            event->accept();
        } else {
            event->ignore();
        }
        return;
    }

    scrollByRows(steps * 3);
    event->accept();
}

void TerminalView::hoverMoveEvent(QHoverEvent* event)
{
    if (!m_session || !terminalOwnsMouse(event->modifiers())
        || m_session->screen().mouseTrackingMode() != TerminalMouseTrackingMode::AnyEvent) {
        event->ignore();
        return;
    }

    sendMouseReport(3, event->position(), false, true);
    event->accept();
}

QPoint TerminalView::cellAt(const QPointF& position) const
{
    if (!m_session || m_cellWidth <= 0.0 || m_cellHeight <= 0.0) {
        return {0, 0};
    }

    const int column = std::clamp(
        static_cast<int>(std::floor(position.x() / m_cellWidth)),
        0,
        m_session->screen().columns() - 1);
    const int visibleRow = std::clamp(
        static_cast<int>(std::floor(position.y() / m_cellHeight)),
        0,
        m_session->screen().rows() - 1);
    return {column, visibleHistoryStart() + visibleRow};
}

QPoint TerminalView::viewportCellAt(const QPointF& position) const
{
    if (!m_session || m_cellWidth <= 0.0 || m_cellHeight <= 0.0) {
        return {0, 0};
    }

    const int column = std::clamp(
        static_cast<int>(std::floor(position.x() / m_cellWidth)),
        0,
        m_session->screen().columns() - 1);
    const int row = std::clamp(
        static_cast<int>(std::floor(position.y() / m_cellHeight)),
        0,
        m_session->screen().rows() - 1);
    return {column, row};
}

bool TerminalView::terminalOwnsMouse(const Qt::KeyboardModifiers& modifiers) const noexcept
{
    return m_session
        && m_session->screen().mouseTrackingMode() != TerminalMouseTrackingMode::None
        && !modifiers.testFlag(Qt::ShiftModifier);
}

void TerminalView::sendMouseReport(int buttonCode, const QPointF& position, bool release, bool motion)
{
    if (!m_session || !m_session->running()) {
        return;
    }

    const QPoint cell = viewportCellAt(position);
    int code = buttonCode;
    const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
    if (modifiers.testFlag(Qt::ShiftModifier)) code += 4;
    if (modifiers.testFlag(Qt::AltModifier)) code += 8;
    if (modifiers.testFlag(Qt::ControlModifier)) code += 16;
    if (motion) code += 32;

    if (m_session->screen().sgrMouseMode()) {
        const QByteArray report = QByteArray("\x1b[<")
            + QByteArray::number(code)
            + ';' + QByteArray::number(cell.x() + 1)
            + ';' + QByteArray::number(cell.y() + 1)
            + (release ? 'm' : 'M');
        m_session->sendBytes(report);
        return;
    }

    const int legacyCode = release ? 3 : code;
    const int encodedX = std::clamp(cell.x() + 1, 1, 223) + 32;
    const int encodedY = std::clamp(cell.y() + 1, 1, 223) + 32;
    QByteArray report("\x1b[M");
    report.append(static_cast<char>(std::clamp(legacyCode + 32, 32, 255)));
    report.append(static_cast<char>(encodedX));
    report.append(static_cast<char>(encodedY));
    m_session->sendBytes(report);
}

bool TerminalView::isCellSelected(int row, int column) const noexcept
{
    if (!m_selectionActive && !m_selecting) {
        return false;
    }

    const auto [start, end] = normalizedSelection();
    const QPoint cell(column, visibleHistoryStart() + row);

    const auto less = [](const QPoint& lhs, const QPoint& rhs) {
        return lhs.y() < rhs.y() || (lhs.y() == rhs.y() && lhs.x() < rhs.x());
    };

    return !less(cell, start) && !less(end, cell);
}

std::pair<QPoint, QPoint> TerminalView::normalizedSelection() const noexcept
{
    const auto less = [](const QPoint& lhs, const QPoint& rhs) {
        return lhs.y() < rhs.y() || (lhs.y() == rhs.y() && lhs.x() < rhs.x());
    };

    if (less(m_selectionEnd, m_selectionStart)) {
        return {m_selectionEnd, m_selectionStart};
    }
    return {m_selectionStart, m_selectionEnd};
}

int TerminalView::visibleHistoryStart() const noexcept
{
    if (!m_session) {
        return 0;
    }

    const TerminalScreen& screen = m_session->screen();
    return std::max(0, screen.historyRows() - screen.rows() - m_scrollbackOffset);
}

void TerminalView::sendSequence(const QString& sequence)
{
    if (!m_session || !m_session->running() || sequence.isEmpty()) {
        return;
    }

    clearSelection();
    scrollToBottom();
    m_session->sendText(sequence);
}

void TerminalView::scrollByRows(int rows)
{
    if (!m_session || rows == 0) {
        return;
    }

    const int maximum = m_session->screen().scrollbackRows();
    const int next = std::clamp(m_scrollbackOffset + rows, 0, maximum);
    if (next == m_scrollbackOffset) {
        return;
    }

    // Scrolling the viewport must not destroy a selection. The selection is
    // stored in history coordinates, so it remains valid even when it moves
    // completely off-screen. Keyboard input still clears it via sendSequence().
    m_scrollbackOffset = next;
    emit scrollbackChanged();
    update();
}

void TerminalView::clampScrollbackOffset()
{
    if (!m_session) {
        if (m_scrollbackOffset != 0) {
            m_scrollbackOffset = 0;
            emit scrollbackChanged();
        }
        return;
    }

    const int maximum = m_session->screen().scrollbackRows();
    const int clamped = std::clamp(m_scrollbackOffset, 0, maximum);
    if (clamped != m_scrollbackOffset) {
        m_scrollbackOffset = clamped;
        emit scrollbackChanged();
    }
}

void TerminalView::updateSelectionAutoScroll(const QPointF& position)
{
    if (!m_selecting || !m_session || m_session->screen().alternateScreenActive()) {
        stopSelectionAutoScroll();
        return;
    }

    // Start auto-scroll slightly before the pointer leaves the viewport. The
    // farther it is outside the edge, the faster the selection advances.
    const qreal edgeZone = std::max<qreal>(24.0, m_cellHeight * 1.6);
    int rowsPerTick = 0;

    if (position.y() < edgeZone) {
        const qreal penetration = edgeZone - position.y();
        const int speed = 1 + static_cast<int>(std::floor(
            penetration / std::max<qreal>(1.0, m_cellHeight * 0.85)));
        rowsPerTick = std::clamp(speed, 1, 10);
    } else if (position.y() > height() - edgeZone) {
        const qreal penetration = position.y() - (height() - edgeZone);
        const int speed = 1 + static_cast<int>(std::floor(
            penetration / std::max<qreal>(1.0, m_cellHeight * 0.85)));
        rowsPerTick = -std::clamp(speed, 1, 10);
    }

    m_selectionAutoScrollRows = rowsPerTick;
    if (rowsPerTick == 0) {
        m_selectionAutoScrollTimer.stop();
        return;
    }

    if (!m_selectionAutoScrollTimer.isActive()) {
        m_selectionAutoScrollTimer.start();
    }
}

void TerminalView::stopSelectionAutoScroll()
{
    m_selectionAutoScrollRows = 0;
    m_selectionAutoScrollTimer.stop();
}

void TerminalView::autoScrollSelection()
{
    if (!m_selecting || !m_session || m_selectionAutoScrollRows == 0) {
        stopSelectionAutoScroll();
        return;
    }

    const int previousOffset = m_scrollbackOffset;
    scrollByRows(m_selectionAutoScrollRows);

    // cellAt() intentionally clamps the pointer to the first/last visible row.
    // After each scroll step that row refers to a different history line, which
    // extends the selection continuously through scrollback.
    setSelectionEnd(cellAt(m_lastSelectionPointer));

    // Stop burning timer wakeups when the beginning/end of history is reached.
    if (m_scrollbackOffset == previousOffset) {
        m_selectionAutoScrollTimer.stop();
    }
}

int TerminalView::searchHighlightAt(int historyRow, int column) const noexcept
{
    if (!m_searchActive || m_searchMatches.empty()) {
        return 0;
    }

    const auto pointLess = [](int lhsRow, int lhsColumn, int rhsRow, int rhsColumn) {
        return lhsRow < rhsRow || (lhsRow == rhsRow && lhsColumn < rhsColumn);
    };

    const auto within = [historyRow, column, &pointLess](const TerminalSearchMatch& match) {
        return !pointLess(historyRow, column, match.startRow, match.startColumn)
            && !pointLess(match.endRow, match.endColumn, historyRow, column);
    };

    if (m_currentSearchIndex >= 0
        && m_currentSearchIndex < static_cast<int>(m_searchMatches.size())
        && within(m_searchMatches.at(static_cast<std::size_t>(m_currentSearchIndex)))) {
        return 2;
    }

    // Matches are produced in terminal-history order. Their end positions are
    // therefore monotonic as well, so skip directly to the first match that
    // can still contain this cell instead of scanning every result per paint.
    const auto candidate = std::lower_bound(
        m_searchMatches.begin(),
        m_searchMatches.end(),
        std::pair<int, int>{historyRow, column},
        [&pointLess](const TerminalSearchMatch& match, const std::pair<int, int>& point) {
            return pointLess(match.endRow, match.endColumn, point.first, point.second);
        });

    if (candidate != m_searchMatches.end() && within(*candidate)) {
        return 1;
    }

    return 0;
}

void TerminalView::rebuildSearchMatches(bool preserveCurrent)
{
    TerminalSearchMatch previousMatch{};
    bool hadPrevious = false;
    if (preserveCurrent
        && m_currentSearchIndex >= 0
        && m_currentSearchIndex < static_cast<int>(m_searchMatches.size())) {
        previousMatch = m_searchMatches.at(static_cast<std::size_t>(m_currentSearchIndex));
        hadPrevious = true;
    }

    m_searchMatches.clear();
    m_currentSearchIndex = -1;

    if (m_session && m_searchActive && !m_searchQuery.isEmpty()) {
        m_searchMatches = findTerminalMatches(
            m_session->screen(),
            m_searchQuery,
            m_searchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

        if (!m_searchMatches.empty()) {
            if (hadPrevious) {
                const auto found = std::find(m_searchMatches.begin(), m_searchMatches.end(), previousMatch);
                if (found != m_searchMatches.end()) {
                    m_currentSearchIndex = static_cast<int>(std::distance(m_searchMatches.begin(), found));
                }
            }

            if (m_currentSearchIndex < 0) {
                // Start at the newest match, which is normally the most useful
                // result when searching command output from the bottom prompt.
                m_currentSearchIndex = static_cast<int>(m_searchMatches.size()) - 1;
            }

            scrollToSearchMatch(m_searchMatches.at(static_cast<std::size_t>(m_currentSearchIndex)));
        }
    }

    emit searchChanged();
    update();
}

void TerminalView::activateSearchMatch(int index)
{
    if (index < 0 || index >= static_cast<int>(m_searchMatches.size())) {
        return;
    }

    m_currentSearchIndex = index;
    scrollToSearchMatch(m_searchMatches.at(static_cast<std::size_t>(index)));
    emit searchChanged();
    update();
}

void TerminalView::scrollToSearchMatch(const TerminalSearchMatch& match)
{
    if (!m_session || m_session->screen().alternateScreenActive()) {
        return;
    }

    const TerminalScreen& screen = m_session->screen();
    const int bottomStart = std::max(0, screen.historyRows() - screen.rows());
    const int centeredStart = std::clamp(
        match.startRow - std::max(0, screen.rows() / 2),
        0,
        bottomStart);
    const int nextOffset = std::clamp(bottomStart - centeredStart, 0, screen.scrollbackRows());

    if (nextOffset != m_scrollbackOffset) {
        m_scrollbackOffset = nextOffset;
        emit scrollbackChanged();
    }
}

void TerminalView::setSelectionEnd(const QPoint& cell)
{
    if (m_selectionEnd == cell) {
        return;
    }

    m_selectionEnd = cell;
    m_selectionActive = true;
    emit selectionChanged();
    update();
}

void TerminalView::wakeCursor()
{
    m_cursorBlinkOn = true;
    m_cursorBlinkTimer.start();
    update();
}

void TerminalView::updateMetrics()
{
    QFont font(m_fontFamily);
    font.setPixelSize(static_cast<int>(std::round(m_fontPixelSize)));
    font.setStyleHint(QFont::Monospace);
    const QFontMetricsF metrics(font);

    m_cellWidth = std::max<qreal>(1.0, std::ceil(metrics.horizontalAdvance(QStringLiteral("M"))));
    m_cellHeight = std::max<qreal>(1.0, std::ceil(metrics.height() + 1.0));
    m_ascent = std::ceil(metrics.ascent());

    emit metricsChanged();
    clearSelection();
    updateTerminalSize();
    update();
}

void TerminalView::updateTerminalSize()
{
    if (!m_session || width() <= 0.0 || height() <= 0.0 || m_cellWidth <= 0.0 || m_cellHeight <= 0.0) {
        return;
    }

    const int columns = std::max(1, static_cast<int>(std::floor(width() / m_cellWidth)));
    const int rows = std::max(1, static_cast<int>(std::floor(height() / m_cellHeight)));
    m_session->resizeTerminal(rows, columns);
}
