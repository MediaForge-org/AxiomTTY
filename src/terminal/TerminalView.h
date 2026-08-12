#pragma once

#include <QPoint>
#include <QPointer>
#include <QQuickPaintedItem>

#include <utility>

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

    Q_INVOKABLE bool copySelection();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void scrollPageUp();
    Q_INVOKABLE void scrollPageDown();
    Q_INVOKABLE void scrollToBottom();

    void paint(QPainter* painter) override;

signals:
    void sessionChanged();
    void fontFamilyChanged();
    void fontPixelSizeChanged();
    void metricsChanged();
    void selectionChanged();
    void scrollbackChanged();
    void selectionCopied();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    [[nodiscard]] QPoint cellAt(const QPointF& position) const;
    void sendSequence(const QString& sequence);
    [[nodiscard]] bool isCellSelected(int row, int column) const noexcept;
    [[nodiscard]] std::pair<QPoint, QPoint> normalizedSelection() const noexcept;
    [[nodiscard]] int visibleHistoryStart() const noexcept;
    void setSelectionEnd(const QPoint& cell);
    void scrollByRows(int rows);
    void clampScrollbackOffset();
    void updateMetrics();
    void updateTerminalSize();

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
};
