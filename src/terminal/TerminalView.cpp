#include "TerminalView.h"

#include "TerminalScreen.h"
#include "TerminalSession.h"

#include <QClipboard>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

TerminalView::TerminalView(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_fontFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family())
{
    setAntialiasing(false);
    setFillColor(QColor());
    setAcceptedMouseButtons(Qt::LeftButton);
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

    const QColor selectionBackground(QStringLiteral("#31537a"));
    const QColor selectionForeground(QStringLiteral("#f7f9fc"));

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

            const bool selected = isCellSelected(rowIndex, column);
            if (selected) {
                background = selectionBackground;
                foreground = selectionForeground;
            }

            const QRectF cellRect(left, top, m_cellWidth + 0.5, m_cellHeight + 0.5);
            if (selected || background != TerminalScreen::defaultBackground()) {
                painter->fillRect(cellRect, background);
            }

            if (!cell.text.isEmpty() && cell.text != QStringLiteral(" ")) {
                QFont font = baseFont;
                font.setBold(cell.style.bold);
                font.setItalic(cell.style.italic);
                font.setUnderline(cell.style.underline);
                painter->setFont(font);
                painter->setPen(foreground);
                painter->drawText(QPointF(left, top + m_ascent), cell.text);
            }
        }
    }

    if (m_scrollbackOffset == 0 && screen.cursorVisible() && m_session->running()) {
        const qreal left = static_cast<qreal>(screen.cursorColumn()) * m_cellWidth;
        const qreal top = static_cast<qreal>(screen.cursorRow()) * m_cellHeight;
        const QRectF cursorRect(left, top + m_cellHeight - 2.0, m_cellWidth, 2.0);
        painter->fillRect(cursorRect, QColor(QStringLiteral("#8fb0f4")));
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
    if (!m_session || !m_session->running()) {
        event->ignore();
        return;
    }

    const Qt::KeyboardModifiers modifiers = event->modifiers();
    const bool ctrl = modifiers.testFlag(Qt::ControlModifier);
    const bool shift = modifiers.testFlag(Qt::ShiftModifier);
    const bool alt = modifiers.testFlag(Qt::AltModifier);

    // Standard desktop copy, without sacrificing Unix ^C:
    // active selection => copy; otherwise Ctrl+C interrupts the foreground job.
    if (ctrl && !alt && event->key() == Qt::Key_C) {
        if (hasSelection()) {
            copySelection();
        } else if (!shift) {
            m_session->sendInterrupt();
        }
        event->accept();
        return;
    }

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

    const auto cursorSequence = [this](const char* normal, const char* application) {
        return QString::fromLatin1(m_session->applicationCursorKeys() ? application : normal);
    };

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: sendSequence(QStringLiteral("\r")); break;
    case Qt::Key_Backspace: sendSequence(QString(1, QChar(0x7f))); break;
    case Qt::Key_Tab: sendSequence(shift ? QStringLiteral("\x1b[Z") : QStringLiteral("\t")); break;
    case Qt::Key_Escape: sendSequence(QStringLiteral("\x1b")); break;
    case Qt::Key_Up: sendSequence(cursorSequence("\x1b[A", "\x1bOA")); break;
    case Qt::Key_Down: sendSequence(cursorSequence("\x1b[B", "\x1bOB")); break;
    case Qt::Key_Right: sendSequence(cursorSequence("\x1b[C", "\x1bOC")); break;
    case Qt::Key_Left: sendSequence(cursorSequence("\x1b[D", "\x1bOD")); break;
    case Qt::Key_Home: sendSequence(cursorSequence("\x1b[H", "\x1bOH")); break;
    case Qt::Key_End: sendSequence(cursorSequence("\x1b[F", "\x1bOF")); break;
    case Qt::Key_Insert: sendSequence(QStringLiteral("\x1b[2~")); break;
    case Qt::Key_Delete: sendSequence(QStringLiteral("\x1b[3~")); break;
    case Qt::Key_PageUp: sendSequence(QStringLiteral("\x1b[5~")); break;
    case Qt::Key_PageDown: sendSequence(QStringLiteral("\x1b[6~")); break;
    case Qt::Key_F1: sendSequence(QStringLiteral("\x1bOP")); break;
    case Qt::Key_F2: sendSequence(QStringLiteral("\x1bOQ")); break;
    case Qt::Key_F3: sendSequence(QStringLiteral("\x1bOR")); break;
    case Qt::Key_F4: sendSequence(QStringLiteral("\x1bOS")); break;
    case Qt::Key_F5: sendSequence(QStringLiteral("\x1b[15~")); break;
    case Qt::Key_F6: sendSequence(QStringLiteral("\x1b[17~")); break;
    case Qt::Key_F7: sendSequence(QStringLiteral("\x1b[18~")); break;
    case Qt::Key_F8: sendSequence(QStringLiteral("\x1b[19~")); break;
    case Qt::Key_F9: sendSequence(QStringLiteral("\x1b[20~")); break;
    case Qt::Key_F10: sendSequence(QStringLiteral("\x1b[21~")); break;
    case Qt::Key_F11: sendSequence(QStringLiteral("\x1b[23~")); break;
    case Qt::Key_F12: sendSequence(QStringLiteral("\x1b[24~")); break;
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

void TerminalView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !m_session) {
        event->ignore();
        return;
    }

    forceActiveFocus();
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
    if (!m_selecting || !(event->buttons() & Qt::LeftButton)) {
        event->ignore();
        return;
    }

    setSelectionEnd(cellAt(event->position()));
    event->accept();
}

void TerminalView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !m_selecting) {
        event->ignore();
        return;
    }

    setSelectionEnd(cellAt(event->position()));
    m_selecting = false;
    m_selectionActive = m_selectionStart != m_selectionEnd;
    emit selectionChanged();
    update();
    event->accept();
}

void TerminalView::wheelEvent(QWheelEvent* event)
{
    if (!m_session || m_session->screen().alternateScreenActive()) {
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

    scrollByRows(steps * 3);
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

    clearSelection();
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
