#pragma once

#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QString>

#include <utility>
#include <vector>

#include "TerminalSearch.h"

class QFocusEvent;
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class TerminalSession;

class TerminalView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject* session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(qreal fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY fontPixelSizeChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth NOTIFY metricsChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight NOTIFY metricsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(int scrollbackOffset READ scrollbackOffset NOTIFY scrollbackChanged)
    Q_PROPERTY(bool searchActive READ searchActive NOTIFY searchChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchChanged)
    Q_PROPERTY(bool searchCaseSensitive READ searchCaseSensitive WRITE setSearchCaseSensitive NOTIFY searchChanged)
    Q_PROPERTY(int searchMatchCount READ searchMatchCount NOTIFY searchChanged)
    Q_PROPERTY(int currentSearchMatch READ currentSearchMatch NOTIFY searchChanged)

public:
    explicit TerminalView(QQuickItem* parent = nullptr);

    [[nodiscard]] QObject* session() const;
    void setSession(QObject* session);

    [[nodiscard]] QString fontFamily() const;
    void setFontFamily(const QString& family);

    [[nodiscard]] qreal fontPixelSize() const noexcept;
    void setFontPixelSize(qreal size);

    [[nodiscard]] qreal cellWidth() const noexcept;
    [[nodiscard]] qreal cellHeight() const noexcept;
    [[nodiscard]] bool hasSelection() const noexcept;
    [[nodiscard]] QString selectedText() const;
    [[nodiscard]] int scrollbackOffset() const noexcept;
    [[nodiscard]] bool searchActive() const noexcept;
    [[nodiscard]] QString searchQuery() const;
    void setSearchQuery(const QString& query);
    [[nodiscard]] bool searchCaseSensitive() const noexcept;
    void setSearchCaseSensitive(bool enabled);
    [[nodiscard]] int searchMatchCount() const noexcept;
    [[nodiscard]] int currentSearchMatch() const noexcept;

    Q_INVOKABLE bool copySelection();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void scrollPageUp();
    Q_INVOKABLE void scrollPageDown();
    Q_INVOKABLE void scrollToBottom();
    Q_INVOKABLE void beginSearch();
    Q_INVOKABLE void endSearch();
    Q_INVOKABLE void findNext();
    Q_INVOKABLE void findPrevious();

    void paint(QPainter* painter) override;

signals:
    void sessionChanged();
    void fontFamilyChanged();
    void fontPixelSizeChanged();
    void metricsChanged();
    void selectionChanged();
    void scrollbackChanged();
    void selectionCopied();
    void searchChanged();
    void searchRequested();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;

private:
    [[nodiscard]] QPoint cellAt(const QPointF& position) const;
    [[nodiscard]] QPoint viewportCellAt(const QPointF& position) const;
    void sendSequence(const QString& sequence);
    void sendMouseReport(int buttonCode, const QPointF& position, bool release, bool motion);
    [[nodiscard]] bool terminalOwnsMouse(const Qt::KeyboardModifiers& modifiers) const noexcept;
    [[nodiscard]] bool isCellSelected(int row, int column) const noexcept;
    [[nodiscard]] std::pair<QPoint, QPoint> normalizedSelection() const noexcept;
    [[nodiscard]] int visibleHistoryStart() const noexcept;
    [[nodiscard]] int searchHighlightAt(int historyRow, int column) const noexcept;
    void rebuildSearchMatches(bool preserveCurrent = true);
    void activateSearchMatch(int index);
    void scrollToSearchMatch(const TerminalSearchMatch& match);
    void setSelectionEnd(const QPoint& cell);
    void scrollByRows(int rows);
    void updateSelectionAutoScroll(const QPointF& position);
    void stopSelectionAutoScroll();
    void autoScrollSelection();
    void clampScrollbackOffset();
    void updateMetrics();
    void updateTerminalSize();
    void wakeCursor();

    QPointer<TerminalSession> m_session;
    QString m_fontFamily;
    qreal m_fontPixelSize{14.0};
    qreal m_cellWidth{9.0};
    qreal m_cellHeight{19.0};
    qreal m_ascent{14.0};

    bool m_selecting{false};
    bool m_selectionActive{false};
    QPoint m_selectionStart{0, 0}; // x = column, y = absolute history row
    QPoint m_selectionEnd{0, 0};
    int m_scrollbackOffset{0};

    bool m_searchActive{false};
    QString m_searchQuery;
    bool m_searchCaseSensitive{false};
    std::vector<TerminalSearchMatch> m_searchMatches;
    int m_currentSearchIndex{-1};

    QTimer m_cursorBlinkTimer;
    bool m_cursorBlinkOn{true};

    QTimer m_selectionAutoScrollTimer;
    QPointF m_lastSelectionPointer{};
    int m_selectionAutoScrollRows{0};
};
