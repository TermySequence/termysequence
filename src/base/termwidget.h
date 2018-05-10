// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"
#include "url.h"
#include "settings/termlayout.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
class QSequentialAnimationGroup;
QT_END_NAMESPACE
class TermStack;
class TermScrollport;
class TermBlinkTimer;
class TermEffectTimer;
class TermScroll;
class SelectionWidget;
class DragIcon;
class InlineCache;

#define NoReportMods (Qt::ShiftModifier|Qt::AltModifier|Qt::ControlModifier)

class TermWidget final: public QWidget, public DisplayIterator
{
    Q_OBJECT
    Q_PROPERTY(int focusFade READ focusFade WRITE setFocusFade)
    Q_PROPERTY(int resizeFade READ resizeFade WRITE setResizeFade)
    Q_PROPERTY(int flashFade READ flashFade WRITE setFlashFade)
    Q_PROPERTY(int cursorBounce READ cursorBounce WRITE setCursorBounce)
    Q_PROPERTY(qreal badgePos READ badgePos WRITE setBadgePos)

    friend class InlineBase;

private:
    TermStack *m_stack;
    TermScrollport *m_scrollport;
    TermScroll *m_scroll;

    bool m_visible = false;
    bool m_focused = false;
    bool m_primary = false;
    bool m_resized = false;
    bool m_commandInd;
    bool m_ownerInd;
    bool m_selectInd;
    bool m_peerInd;
    bool m_lockInd;
    bool m_alertInd;
    bool m_followInd;
    bool m_leaderInd;
    bool m_mouseMode;
    bool m_mouseReport;
    bool m_altScroll;
    bool m_focusEvents;
    bool m_badgeRunning;

    bool m_selecting = false;
    bool m_dragging = false;
    unsigned m_nFills = 0;

    QPoint m_dragPoint;
    int m_dragVector;
    int m_tripleId = 0;
    QPoint m_triplePoint;

    InlineCache *m_cache;
    SelectionWidget *m_sel;
    TermLayout::Fill *m_fills = nullptr;

    TermBlinkTimer *m_blink;
    TermEffectTimer *m_effects;
    int m_focusFade = 255;
    int m_resizeFade = 255;
    int m_flashFade = 0;
    int m_cursorBounce = 0;
    qreal m_badgePos;
    QPropertyAnimation *m_focusAnim;
    QPropertyAnimation *m_resizeAnim;
    QPropertyAnimation *m_flashAnim;
    QPropertyAnimation *m_cursorAnim;
    QSequentialAnimationGroup *m_badgeSag;

    QString m_resizeStr1;
    QString m_resizeStr2;
    QRect m_resizeRect;
    QRect m_resizeRect1;
    QRect m_resizeRect2;

    QString m_indexStr;
    QRectF m_indexRect;
    QPointF m_commandPoint, m_selectPoint, m_ownerPoint, m_lockPoint;
    qreal m_indicatorScale;
    QRect m_dragBounds;

    QString m_badgeStr;

    DragIcon *m_drag = nullptr;

private:
    void updateSize(const QSize &size);
    void updateIndexBounds(const QSize &size);
    void updateResizeBounds(const QSize &size, const QSize &emulatorSize);
    void updateBadgeBounds(int height);

    QPointF pixelPosition(int x, int y) const;
    QPoint screenPosition(int x, int y) const;
    QPoint nearestPosition(const QPoint &p) const;
    QPoint selectPosition() const;

    void paintBox(QPainter &painter, const DisplayCell &i, CellState &state);

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void wheelEvent(QWheelEvent *event);
    void timerEvent(QTimerEvent *event);

private slots:
    void redisplay();
    void refocus();
    void reindex(int index);
    void refont(const QFont &font);

    void blink();
    void bell();
    void handleOffsetChanged();
    void handleSizeChanged(QSize size);
    void handleFocusEffectChanged(int focusEffect);
    void handleResizeEffectChanged(int resizeEffect);
    void handleSettingsChanged();
    void handleBadgeChanged();
    void handleFlagsChanged(Tsq::TermFlags flags);
    void handleFillsChanged(const QString &fillsStr);
    void handleOwnershipChanged(bool ours);
    void handleIndicators();
    void handleUrlChanged(const TermUrl &tu);
    void handleContextMenuRequest();
    void handleCursorHighlight();

    void dragTimerCallback();

public:
    TermWidget(TermStack *stack, TermScrollport *scrollport, TermScroll *scroll, QWidget *parent);
    ~TermWidget();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    inline int focusFade() const { return m_focusFade; }
    void setFocusFade(int focusFade);
    inline int resizeFade() const { return m_resizeFade; }
    void setResizeFade(int resizeFade);
    inline int flashFade() const { return m_flashFade; }
    void setFlashFade(int flashFade);
    inline int cursorBounce() const { return m_cursorBounce; }
    void setCursorBounce(int cursorBounce);
    inline qreal badgePos() const { return m_badgePos; }
    void setBadgePos(qreal badgePos);

    QPoint mousePosition() const;

    void updateViewportSize(const QSize &size);
    void startSelecting(const QPoint &pos);
    void finishSelecting(const QPoint &pos);
    void moveSelecting(const QPoint &pos);

    static void initialize();
};

inline void
TermWidget::setResizeFade(int resizeFade)
{
    m_resizeFade = resizeFade;
    update();
}

inline void
TermWidget::setFlashFade(int flashFade)
{
    m_flashFade = flashFade;
    update();
}

inline void
TermWidget::setCursorBounce(int cursorBounce)
{
    m_cursorBounce = cursorBounce;
    update();
}

inline void
TermWidget::setBadgePos(qreal badgePos)
{
    m_badgePos = badgePos;
    update();
}
