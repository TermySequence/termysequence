// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/keys.h"
#include "app/logging.h"
#include "termwidget.h"
#include "listener.h"
#include "manager.h"
#include "stack.h"
#include "term.h"
#include "termformat.h"
#include "buffers.h"
#include "screen.h"
#include "scrollport.h"
#include "selection.h"
#include "blinktimer.h"
#include "effecttimer.h"
#include "scroll.h"
#include "mainwindow.h"
#include "selwidget.h"
#include "dragicon.h"
#include "baseinline.h"
#include "highlight.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/keymap.h"
#include "lib/sequences.h"

#include <QMenu>
#include <QTimer>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QMimeData>
#include <QtMath>
#include <QResizeEvent>
#include <QApplication>

#define TR_TEXT1 TL("window-text", "Viewport: %1 x %2")
#define TR_TEXT2 TL("window-text", "Emulator: %1 x %2")

static QPainterPath s_commandPath, s_selectPath,
    s_peerPath, s_ownerPath, s_lockPath, s_alertPath, s_leaderPath, s_followPath;

TermWidget::TermWidget(TermStack *stack, TermScrollport *scrollport,
                       TermScroll *scroll, QWidget *parent) :
    QWidget(parent),
    DisplayIterator(scrollport->term()),
    m_stack(stack),
    m_scrollport(scrollport),
    m_scroll(scroll),
    m_blink(g_listener->blink()),
    m_effects(g_listener->effects())
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::IBeamCursor);
    setAcceptDrops(true);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::PreventContextMenu);

    m_cache = new InlineCache(this);
    m_sel = new SelectionWidget(scrollport, this);

    m_focusAnim = new QPropertyAnimation(this, B("focusFade"), this);
    m_focusAnim->setDuration(BLINK_TIME);
    m_focusAnim->setEndValue(0);
    m_resizeAnim = new QPropertyAnimation(this, B("resizeFade"), this);
    m_resizeAnim->setDuration(BLINK_TIME);
    m_resizeAnim->setEndValue(0);
    m_flashAnim = new QPropertyAnimation(this, B("flashFade"), this);
    m_flashAnim->setEasingCurve(QEasingCurve::OutQuint);
    m_flashAnim->setDuration(BLINK_TIME);
    m_flashAnim->setStartValue(200);
    m_flashAnim->setEndValue(0);
    m_cursorAnim = new QPropertyAnimation(this, B("cursorBounce"), this);
    QEasingCurve ec(QEasingCurve::OutElastic);
    ec.setAmplitude(2.0);
    m_cursorAnim->setEasingCurve(ec);
    m_cursorAnim->setDuration(BLINK_TIME * 2);
    m_cursorAnim->setEndValue(0);
    m_badgeSag = new QSequentialAnimationGroup(this);
    m_badgeSag->addPause(BADGE_RATE_PAUSE);
    auto *badgeAnim = new QPropertyAnimation(this, B("badgePos"));
    badgeAnim->setStartValue(0);
    m_badgeSag->addAnimation(badgeAnim);
    m_badgeSag->addPause(BADGE_RATE_PAUSE);
    badgeAnim = new QPropertyAnimation(this, B("badgePos"));
    badgeAnim->setEndValue(0);
    m_badgeSag->addAnimation(badgeAnim);

    m_blink->addViewport(m_scrollport);
    m_effects->addViewport(m_scrollport);

    connect(m_term->buffers(), SIGNAL(contentChanged()), SLOT(redisplay()));
    connect(m_term, SIGNAL(sizeChanged(QSize)), SLOT(handleSizeChanged(QSize)));
    connect(m_blink, SIGNAL(timeout()), SLOT(blink()));
    connect(m_scrollport, SIGNAL(offsetChanged(int)), SLOT(handleOffsetChanged()));
    connect(m_scrollport, SIGNAL(flagsChanged(Tsq::TermFlags)), SLOT(handleFlagsChanged(Tsq::TermFlags)));
    connect(m_scrollport, SIGNAL(searchUpdate()), SLOT(redisplay()));
    connect(m_scrollport, SIGNAL(resizeEffectChanged(int)), SLOT(handleResizeEffectChanged(int)));
    connect(m_scrollport, SIGNAL(focusEffectChanged(int)), SLOT(handleFocusEffectChanged(int)));
    connect(m_scrollport, SIGNAL(selectedUrlChanged(const TermUrl&)), SLOT(handleUrlChanged(const TermUrl&)));
    connect(m_scrollport, SIGNAL(contextMenuRequest()), SLOT(handleContextMenuRequest()));
    connect(m_stack->manager(), SIGNAL(activeChanged(bool)), SLOT(refocus()));
    connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(update()));
    connect(m_term, SIGNAL(paletteChanged()), SLOT(update()));
    connect(m_term, SIGNAL(ownershipChanged(bool)), SLOT(handleOwnershipChanged(bool)));
    connect(m_term, SIGNAL(peerChanged()), SLOT(handleIndicators()));
    connect(m_term, SIGNAL(alertChanged()), SLOT(handleIndicators()));
    connect(m_term, SIGNAL(inputChanged()), SLOT(handleIndicators()));
    connect(m_term, SIGNAL(miscSettingsChanged()), SLOT(handleSettingsChanged()));
    connect(m_term, SIGNAL(fillsChanged(const QString&)), SLOT(handleFillsChanged(const QString&)));
    connect(m_term, SIGNAL(bellRang()), SLOT(bell()));
    connect(m_term, SIGNAL(contentChanged()), SLOT(redisplay()));
    connect(m_term, SIGNAL(cursorHighlight()), SLOT(handleCursorHighlight()));
    connect(m_term->format(), SIGNAL(badgeChanged()), SLOT(handleBadgeChanged()));
    connect(g_global, SIGNAL(mainSettingsChanged()), SLOT(handleSettingsChanged()));
    connect(g_listener, SIGNAL(presModeChanged(bool)), SLOT(handleSettingsChanged()));

    handleSettingsChanged();
    handleFillsChanged(m_term->fills());
    refont(m_term->font());
    refocus();
}

TermWidget::~TermWidget()
{
    delete [] m_fills;
    delete m_cache;
}

void
TermWidget::blink()
{
    if (m_scrollport->updateBlink)
        update();
}

void
TermWidget::redisplay()
{
    if (m_term->updating() || !m_visible)
        return;

    m_cache->process(m_scrollport->regions());

    if (m_sel->active()) {
        m_sel->raise();
        m_sel->startAnimation();
    }

    calculateCells(m_scrollport, m_mouseMode);
    m_blink->setBlinkEffect(m_scrollport);
    update();
}

void
TermWidget::refocus()
{
    TermManager *manager = m_stack->manager();
    m_primary = manager->active() && m_focused;

    if (m_focused)
        manager->setActiveStack(m_stack);
    if (m_primary)
        manager->setActiveTerm(m_term, m_scrollport);

    m_scrollport->setPrimary(m_primary);
    m_scrollport->setFocused(m_focused);

    if (m_sel->active()) {
        if (m_primary)
            m_sel->startAnimation();
        else
            m_sel->stopAnimation();
    }

    m_blink->setBlinkEffect(m_scrollport);
    m_effects->setFocusEffect(m_scrollport);
    update();
}

void
TermWidget::reindex(int index)
{
    m_indexStr = index ? QString::number(index) : g_mtstr;
    updateIndexBounds(size());
    update();
}

static inline bool indexOk() {
    return g_global->mainIndex() && !(g_listener->presMode() && g_global->presIndex());
}
static inline bool indicatorOk(const ProfileSettings *profile) {
    return profile->mainIndicator() && !(g_listener->presMode() && g_global->presIndicator());
}
static inline bool badgeOk() {
    return !(g_listener->presMode() && g_global->presBadge());
}

void
TermWidget::handleFlagsChanged(Tsq::TermFlags flags)
{
    const auto *profile = m_term->profile();
    bool indok = indicatorOk(profile);

    m_commandInd = flags & (Tsqt::CommandMode) && indok;
    m_selectInd = flags & (Tsqt::SelectMode) && indok;
    m_lockInd = flags & (Tsq::SoftScrollLock|Tsq::HardScrollLock) && indok;

    m_altScroll = (flags & Tsqt::AltScroll) == Tsqt::AltScroll &&
        m_term->ours() && profile->scrollMode();
    m_focusEvents = (flags & Tsq::FocusEventMode) &&
        m_term->ours() && profile->focusMode();

    m_mouseMode = flags & Tsq::MouseModeMask;
    m_mouseReport = m_mouseMode && m_term->ours() && profile->mouseMode();
    update();
}

void
TermWidget::handleOwnershipChanged(bool ours)
{
    const auto *profile = m_term->profile();

    m_ownerInd = !ours && indicatorOk(profile);

    m_altScroll = (m_term->flags() & Tsqt::AltScroll) == Tsqt::AltScroll &&
        ours && profile->scrollMode();
    m_focusEvents = (m_term->flags() & Tsq::FocusEventMode) &&
        ours && profile->focusMode();

    m_mouseReport = m_mouseMode && ours && profile->mouseMode();
    update();
}

void
TermWidget::handleFillsChanged(const QString &fillsStr)
{
    TermLayout layout;
    layout.parseFills(fillsStr);
    delete [] m_fills;
    m_fills = new TermLayout::Fill[m_nFills = layout.fills().size()];
    for (unsigned i = 0; i < m_nFills; ++i)
        m_fills[i] = layout.fills().at(i);
    update();
}

void
TermWidget::handleIndicators()
{
    bool iok = indicatorOk(m_term->profile());
    m_peerInd = m_term->peer() && iok;
    m_alertInd = m_term->alert() && iok;
    m_followInd = m_term->inputFollower() && iok;
    m_leaderInd = m_term->inputLeader() && iok;
    update();
}

void
TermWidget::updateBadgeBounds(int height)
{
    if (m_badgeRunning) {
        m_badgeRunning = false;
        m_badgeSag->stop();
    }

    m_badgePos = 0;

    if (!g_global->badgeEffect())
        return;

    int excess = m_metrics.width(m_badgeStr) * BADGE_SCALE - height;
    if (excess > 0) {
        excess /= BADGE_SCALE;
        int duration = excess * g_global->badgeRate() / m_cellSize.width();
        auto *badgeAnim = static_cast<QVariantAnimation*>(m_badgeSag->animationAt(1));
        badgeAnim->setEndValue(-excess);
        badgeAnim->setDuration(duration);
        badgeAnim = static_cast<QVariantAnimation*>(m_badgeSag->animationAt(3));
        badgeAnim->setStartValue(-excess);
        badgeAnim->setDuration(duration);
        m_badgeRunning = true;
        m_badgeSag->start();
    }
}

void
TermWidget::handleBadgeChanged()
{
    m_badgeStr = badgeOk() ? m_term->format()->badge() : QString();
    update();

    if (m_visible)
        updateBadgeBounds(height());
    else
        m_resized = true;
}

void
TermWidget::handleSettingsChanged()
{
    if (indexOk()) {
        connect(m_stack, SIGNAL(indexChanged(int)), SLOT(reindex(int)), Qt::UniqueConnection);
        reindex(m_stack->index());
    } else {
        disconnect(m_stack, SIGNAL(indexChanged(int)), this, SLOT(reindex(int)));
        reindex(0);
    }

    handleBadgeChanged();
    handleFlagsChanged(m_scrollport->flags());
    handleOwnershipChanged(m_term->ours());
    handleIndicators();
}

void
TermWidget::refont(const QFont &font)
{
    setDisplayFont(font);

    qCDebug(lcLayout) << this << "cell size is" << m_cellSize << "ascent is" << m_ascent;

    if (m_visible)
        updateSize(size());
    else
        m_resized = true;

    m_cache->calculateCellSize();
    m_sel->calculateCellSize();
    m_cursorAnim->setStartValue(m_cellSize.width() / 2);
    updateGeometry();
}

void
TermWidget::bell()
{
    switch (g_global->mainBell()) {
    case BellFlash:
        m_flashAnim->start();
        break;
    case BellBounce:
        m_cursorAnim->start();
        break;
    default:
        break;
    }
}

void
TermWidget::handleFocusEffectChanged(int focusEffect)
{
    if (focusEffect == 0 || focusEffect > EFFECT_FOCUS_FADE) {
        if (m_focusAnim->state() != QAbstractAnimation::Stopped)
            m_focusAnim->stop();

        setFocusFade(255);
        update();
    }
    else if (focusEffect == EFFECT_FOCUS_FADE)
        m_focusAnim->start();
}

void
TermWidget::handleResizeEffectChanged(int resizeEffect)
{
    if (resizeEffect == 0 || resizeEffect > EFFECT_RESIZE_FADE) {
        if (m_resizeAnim->state() != QAbstractAnimation::Stopped)
            m_resizeAnim->stop();

        m_resizeFade = 255;
        update();
    }
    else if (resizeEffect == EFFECT_RESIZE_FADE)
        m_resizeAnim->start();
}

void
TermWidget::updateIndexBounds(const QSize &size)
{
    qreal cw = m_cellSize.width();
    qreal ch = m_cellSize.height();
    qreal o = (size.width() / INDEX_SCALE - cw) / 2.0;
    qreal sw = cw * (1.0 + m_indexStr.length()) / 2.0;
    qreal y = (ch - cw) * INDEX_SCALE / 2.0;

    m_indexRect.setRect(o, 0.0, cw, ch);

    m_ownerPoint = QPointF((o - sw) * INDEX_SCALE, y);
    m_lockPoint = QPointF((o + sw) * INDEX_SCALE, y);

    y = size.height() - cw * INDEX_SCALE;
    if (y < ch * INDEX_SCALE)
        y = ch * INDEX_SCALE;

    sw = size.width() - cw * INDEX_SCALE - ch * (BADGE_SCALE + 2.0);
    m_commandPoint = QPointF(sw, y);
    sw -= cw * INDEX_SCALE;
    m_selectPoint = QPointF(sw, y);

    m_indicatorScale = cw * INDEX_SCALE / 100.0;

    ch *= 5;
    y = (size.height() - ch) / 2;
    m_dragBounds.setRect(0, y, width(), ch);
}

void
TermWidget::updateResizeBounds(const QSize &size, const QSize &emulatorSize)
{
    int w = size.width();
    int h = size.height();

    QSize viewportSize(w / m_cellSize.width(), h / m_cellSize.height());

    m_resizeStr1 = TR_TEXT1.arg(viewportSize.width()).arg(viewportSize.height());
    m_resizeStr2 = TR_TEXT2.arg(emulatorSize.width()).arg(emulatorSize.height());

    int cw = m_cellSize.width();
    int ch = m_cellSize.height();
    int rw1 = m_metrics.width(m_resizeStr1) + 2 * cw;
    int rw2 = m_metrics.width(m_resizeStr2) + 2 * cw;
    int rw = (rw1 > rw2) ? rw1 : rw2;
    int rh = 3 * ch;

    rh += (rh & 1);

    m_resizeRect.setRect((w - rw) / 2, (h - rh) / 2, rw, rh);
    m_resizeRect1.setRect((w - rw) / 2, (h - rh) / 2, rw, rh / 2);
    m_resizeRect2.setRect((w - rw) / 2, (h - rh) / 2 + rh / 2, rw, rh / 2);
}

void
TermWidget::handleSizeChanged(QSize emulatorSize)
{
    if (m_selecting) {
        m_term->buffers()->selection()->clear();
        m_selecting = false;
    }

    // Activate the resize effect
    updateResizeBounds(size(), emulatorSize);
    m_effects->setResizeEffect(m_scrollport);
}

void
TermWidget::handleOffsetChanged()
{
    if (m_selecting)
        m_term->buffers()->selection()->setFloat(selectPosition());

    redisplay();
}

void
TermWidget::updateViewportSize(const QSize &size)
{
    QSize viewportSize(size.width() / m_cellSize.width(), size.height() / m_cellSize.height());
    m_scrollport->setSize(viewportSize);
}

void
TermWidget::updateSize(const QSize &size)
{
    qCDebug(lcLayout) << this << "resize to" << size;

    int w = size.width();
    int h = size.height();
    qreal cw = m_cellSize.width();
    qreal ch = m_cellSize.height();

    QSize viewportSize(w / cw, h / ch);

    // Calculate message bounds
    updateIndexBounds(size);
    updateResizeBounds(size, m_term->screen()->size());
    updateBadgeBounds(size.height());

    // Update viewport size
    if (m_scrollport->setSize(viewportSize)) {
        // Activate the resize effect
        m_effects->setResizeEffect(m_scrollport);
    }
}

void
TermWidget::paintBox(QPainter &painter, const DisplayCell &i, CellState &state)
{
    QRgb fgval, bgval;

    // Colors
    if (i.flags & Tsq::Fg) {
        fgval = (i.flags & Tsq::FgIndex) ? m_term->palette().std(i.fg) : i.fg;
    } else {
        fgval = state.fg;
    }

    if (i.flags & Tsq::Bg) {
        bgval = (i.flags & Tsq::BgIndex) ? m_term->palette().std(i.bg) : i.bg;
    } else {
        bgval = state.bg;
    }

    if (i.flags & Tsq::Inverse) {
        QRgb tmp = bgval;
        bgval = fgval;
        fgval = tmp;
    }
    if (i.flags & Tsqt::Special) {
        fgval = m_term->palette().specialFg(i.flags, fgval);
        bgval = m_term->palette().specialBg(i.flags, bgval);
    }
    if (i.flags & Tsqt::Selected) {
        fgval = bgval;
    }

    // Drawing
    qreal x = i.rect.x();
    qreal y = i.rect.y();
    qreal h = m_cellSize.height();
    qreal w = i.rect.width();

    if (i.lineFlags & Tsq::DblWidthLine) {
        x *= 2;
        w *= 2;
    }

    int line = 1 + (int)h / CURSOR_BOX_INCREMENT;
    qreal half = line / 2.0;
    QRectF box(x + half, y + half, w - line, h - line);

    QPen savedPen(painter.pen());
    QPen pen(savedPen);
    pen.setColor(fgval);
    pen.setWidth(line);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);

    painter.setPen(pen);
    painter.drawRect(box);
    painter.setPen(savedPen);
}

void
TermWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    CellState state(m_scrollport->textBlink ? Tsq::Blink|Tsq::Invisible : Tsq::Invisible);

    QColor fg = state.fg = m_term->fg();
    state.bg = m_term->bg();

    QColor indexColor(fg);
    indexColor.setAlphaF(0.2);
    painter.setPen(indexColor);
    painter.setFont(m_font);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw stack index background
    if (!m_indexStr.isEmpty()) {
        painter.scale(INDEX_SCALE, INDEX_SCALE);
        painter.drawText(m_indexRect, Qt::AlignCenter, m_indexStr);
        painter.resetTransform();
    }

    // Draw command indicator
    if (m_commandInd) {
        painter.translate(m_commandPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_commandPath, indexColor);
        painter.resetTransform();
    }

    // Draw select indicator
    if (m_selectInd) {
        painter.translate(m_commandInd ? m_selectPoint : m_commandPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_selectPath, indexColor);
        painter.resetTransform();
    }

    // Draw owner/peer indicator
    if (m_peerInd) {
        painter.translate(m_ownerPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_peerPath, indexColor);
        painter.resetTransform();
    } else if (m_ownerInd) {
        painter.translate(m_ownerPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_ownerPath, indexColor);
        painter.resetTransform();
    }

    // Draw mode indicator
    if (m_lockInd) {
        painter.translate(m_lockPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_lockPath, indexColor);
        painter.resetTransform();
    } else if (m_leaderInd) {
        painter.translate(m_lockPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_leaderPath, indexColor);
        painter.resetTransform();
    } else if (m_followInd) {
        painter.translate(m_lockPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_followPath, indexColor);
        painter.resetTransform();
    } else if (m_alertInd) {
        painter.translate(m_lockPoint);
        painter.scale(m_indicatorScale, m_indicatorScale);
        painter.fillPath(s_alertPath, indexColor);
        painter.resetTransform();
    }

    // Draw badge text
    if (!m_badgeStr.isEmpty()) {
        painter.translate(width(), 0);
        painter.rotate(90.0);
        painter.scale(BADGE_SCALE, BADGE_SCALE);
        painter.drawText(QPointF(m_badgePos, m_ascent), m_badgeStr);
        painter.resetTransform();
    }

    // Draw emulator contents
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (const DisplayCell &cell: m_displayCells) {
        paintTerm(painter, cell, state);
    }

    // Draw fills
    if (m_nFills) {
        QPen pen;
        pen.setWidth(1 + (int)m_cellSize.height() / MARGIN_INCREMENT);
        unsigned i = 0;
        do {
            const auto *fill = m_fills + i;
            QColor color = PALETTE_IS_ENABLED(fill->second) ?
                m_term->palette().at(fill->second) : m_term->fg();
            color.setAlphaF(0.4);
            pen.setColor(color);
            painter.setPen(pen);
            int x = m_cellSize.width() * fill->first;
            painter.drawLine(x, 0, x, height());
        } while (++i < m_nFills);
    }

    // Draw mouse position
    if (m_mouseMode) {
        paintBox(painter, m_mouseCell, state);
    }

    // Draw cursor
    if (!m_scrollport->cursorBlink && !m_scrollport->scrolled() && !m_term->overlayActive())
    {
        DisplayCell i(m_cursorCell);
        if (m_cursorBounce) {
            i.rect.translate(m_cursorBounce, 0.0);
            i.point.rx() += m_cursorBounce;
        }
        if (m_scrollport->focused()) {
            i.flags |= Tsqt::SolidCursor;
            paintTerm(painter, i, state);
        } else {
            paintBox(painter, i, state);
        }
    }

    // Draw flash effect
    if (m_flashFade) {
        QColor color(fg);
        color.setAlpha(m_flashFade);
        painter.fillRect(rect(), color);
    }

    // Draw resize popup
    if (m_scrollport->resizeEffect()) {
        QFont font = painter.font();
        font.setWeight(m_fontWeight);
        font.setUnderline(false);
        painter.setFont(font);

        QPalette pal = QGuiApplication::palette();
        QColor color = pal.color(QPalette::Highlight);
        color.setAlpha(m_resizeFade);
        painter.fillRect(m_resizeRect1, color);

        color = pal.color(QPalette::Window);
        color.setAlpha(m_resizeFade);
        painter.fillRect(m_resizeRect2, color);

        color = pal.color(QPalette::HighlightedText);
        color.setAlpha(m_resizeFade);
        painter.setPen(color);
        painter.drawText(m_resizeRect1, Qt::AlignCenter, m_resizeStr1);

        color = pal.color(QPalette::WindowText);
        color.setAlpha(m_resizeFade);
        painter.setPen(color);
        painter.drawText(m_resizeRect2, Qt::AlignCenter, m_resizeStr2);

        color = pal.color(QPalette::Dark);
        color.setAlpha(m_resizeFade);
        painter.setPen(color);
        painter.drawRect(m_resizeRect);
        painter.drawRect(m_resizeRect2);
    }

    // Draw focus effect
    if (m_scrollport->focusEffect()) {
        fg.setAlphaF(m_focusFade * 0.4 / 255.0);
        painter.setPen(fg);
        painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
}

bool
TermWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        m_scrollport->keyPressEvent(static_cast<QKeyEvent *>(event));
        m_blink->setBlinkEffect(m_scrollport);
        if (m_sel->active())
            m_sel->startAnimation();
        if (m_badgeRunning && m_badgeSag->state() == QAbstractAnimation::Stopped)
            m_badgeSag->start();
        return true;
    case QEvent::KeyRelease:
        return true;
    default:
        return QWidget::event(event);
    }
}

QPointF
TermWidget::pixelPosition(int x, int y) const
{
    if (x < 0)
        x = 0;
    if (x >= width())
        x = width() - 1;
    if (y < 0)
        y = 0;
    if (y >= height())
        y = height() - 1;

    return QPointF(x / m_cellSize.width(), y / m_cellSize.height());
}

inline QPoint
TermWidget::screenPosition(int x, int y) const
{
    return m_scrollport->screenPosition(pixelPosition(x, y));
}

inline QPoint
TermWidget::nearestPosition(const QPoint &p) const
{
    return m_scrollport->nearestPosition(pixelPosition(p.x(), p.y()));
}

inline QPoint
TermWidget::selectPosition() const
{
    return nearestPosition(m_dragPoint);
}

QPoint
TermWidget::mousePosition() const
{
    QPoint p = mapFromGlobal(QCursor::pos());
    return m_scrollport->nearestPosition(pixelPosition(p.x(), p.y()));
}

void
TermWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!m_drag)
        m_drag = new DragIcon(this);

    if (event->mimeData()->hasText() || event->mimeData()->hasUrls()) {
        m_drag->setAccepted(true);
        event->acceptProposedAction();
    } else {
        m_drag->setAccepted(false);
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }

    m_drag->setGeometry(m_dragBounds);
    m_drag->raise();
    m_drag->show();
}

void
TermWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    if (m_drag)
        m_drag->setVisible(false);
}

void
TermWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        m_stack->manager()->terminalFileDrop(m_term, event->mimeData());
    } else if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        m_term->pushInput(m_stack->manager(), event->mimeData()->text());
    } else {
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }

    if (m_drag)
        m_drag->hide();
}

void
TermWidget::handleUrlChanged(const TermUrl &tu)
{
    m_cache->updateUrl(tu);
}

void
TermWidget::handleContextMenuRequest()
{
    QPoint mid(width() / 2, height() / 4);
    m_scrollport->setClickPoint(m_cellSize.height(), height(), mid.y());

    QMenu *m = m_stack->manager()->parent()->getTermPopup(m_term);
    m->popup(mapToGlobal(mid));
}

void
TermWidget::handleCursorHighlight()
{
    new CursorHighlight(this, this, m_scrollport);
}

static inline Tsq::MouseEventFlags
mousePressFlags(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        return 1;
    case Qt::MiddleButton:
        return 2;
    case Qt::RightButton:
        return 3;
    default:
        return 0;
    }
}

static inline Tsq::MouseEventFlags
mouseWheelFlags(QWheelEvent *event, bool down)
{
    Tsq::MouseEventFlags result = down ? 5 : 4;
    return result;
}

static inline Tsq::MouseEventFlags
mouseMotionFlags(QMouseEvent *event)
{
    Tsq::MouseEventFlags result = Tsq::MouseMotion;

    if (event->buttons() & Qt::LeftButton)
        result |= 1;
    else if (event->buttons() & Qt::MiddleButton)
        result |= 2;
    else if (event->buttons() & Qt::RightButton)
        result |= 3;

    return result;
}

void
TermWidget::startSelecting(const QPoint &pos)
{
    m_selecting = true;
    m_dragPoint = pos;
}

void
TermWidget::finishSelecting(const QPoint &pos)
{
    m_selecting = m_dragging = false;
    m_dragPoint = pos;
}

void
TermWidget::moveSelecting(const QPoint &pos)
{
    if (pos.y() < 0) {
        m_dragVector = -1;
        if (!m_dragging) {
            m_dragging = true;
            QTimer::singleShot(DRAG_SELECT_TIME, this, SLOT(dragTimerCallback()));
        }
    }
    else if (pos.y() >= height()) {
        m_dragVector = 1;
        if (!m_dragging) {
            m_dragging = true;
            QTimer::singleShot(DRAG_SELECT_TIME, this, SLOT(dragTimerCallback()));
        }
    }
    else {
        m_dragging = false;
    }

    m_dragPoint = pos;
    m_term->buffers()->selection()->setFloat(selectPosition());
}

void
TermWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if ((!m_mouseReport || (event->modifiers() & NoReportMods)) &&
        event->button() == Qt::LeftButton)
    {
        m_term->buffers()->selection()->selectWordAt(
            nearestPosition(event->pos()));

        if (m_tripleId)
            killTimer(m_tripleId);
        m_tripleId = startTimer(QApplication::doubleClickInterval());
        m_triplePoint = event->pos();
    }
}

void
TermWidget::mousePressEvent(QMouseEvent *event)
{
    int button = event->button();
    switch (button) {
    case Qt::LeftButton:
    case Qt::RightButton:
    case Qt::MiddleButton:
        if (m_mouseReport && !(event->modifiers() & NoReportMods)) {
            Tsq::MouseEventFlags flags = mousePressFlags(event);
            QPoint p = screenPosition(event->x(), event->y());

            if (p.x() >= 0)
                m_term->mouseEvent(flags, p);

            goto out;
        }
        break;
    default:
        QKeyEvent ke(QEvent::KeyPress, button|KEY_BUTTON_BIT, event->modifiers());
        this->event(&ke);
        return;
    }

    switch (button) {
    case Qt::LeftButton:
        if (m_tripleId) {
            killTimer(m_tripleId);
            m_tripleId = 0;

            QPoint dist = event->pos() - m_triplePoint;
            if (dist.manhattanLength() < QApplication::startDragDistance())
                m_term->buffers()->selection()->selectLineAt(
                    nearestPosition(event->pos()));
        }
        else {
            startSelecting(event->pos());
            m_term->buffers()->selection()->setAnchor(selectPosition());
        }
        break;
    case Qt::MiddleButton:
        m_stack->manager()->actionPasteSelectBuffer(m_term->idStr());
        break;
    case Qt::RightButton:
        MainWindow *window = m_stack->manager()->parent();
        InlineBase *child = qobject_cast<InlineBase*>(childAt(event->pos()));
        auto m = child ? child->getInlinePopup(window) : window->getTermPopup(m_term);

        m_scrollport->setClickPoint(m_cellSize.height(), height(), event->y());
        m->popup(event->globalPos());
        break;
    }
out:
    m_blink->setBlinkEffect(m_scrollport);
    if (m_badgeRunning && m_badgeSag->state() == QAbstractAnimation::Stopped)
        m_badgeSag->start();
}

void
TermWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        finishSelecting(event->pos());
        m_term->buffers()->selection()->finish(selectPosition());
    }
    else if (m_mouseReport && !(event->modifiers() & NoReportMods)) {
        Tsq::MouseEventFlags flags = mousePressFlags(event);
        QPoint p = screenPosition(event->x(), event->y());

        if (flags && p.x() >= 0) {
            flags |= Tsq::MouseRelease;
            m_term->mouseEvent(flags, p);
        }
    }
}

void
TermWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_selecting) {
        moveSelecting(event->pos());
    }
    else if (m_mouseReport && !(event->modifiers() & NoReportMods)) {
        QPoint p = screenPosition(event->x(), event->y());

        if (p.x() >= 0 && p != m_term->mousePos()) {
            Tsq::MouseEventFlags flags = mouseMotionFlags(event);
            m_term->mouseEvent(flags, p);
        }
    }
}

void
TermWidget::dragTimerCallback()
{
    if (m_dragging) {
        int req = m_dragVector < 0 ? SCROLLREQ_LINEUP : SCROLLREQ_LINEDOWN;
        emit m_scrollport->scrollRequest(req);

        // selection updated in handleOffsetChanged

        QTimer::singleShot(DRAG_SELECT_TIME, this, SLOT(dragTimerCallback()));
    }
}

void
TermWidget::timerEvent(QTimerEvent *)
{
    killTimer(m_tripleId);
    m_tripleId = 0;
}

void
TermWidget::focusInEvent(QFocusEvent *event)
{
    m_focused = true;
    refocus();
    if (m_focusEvents)
        m_term->pushInput(m_stack->manager(), TSQ_FOCUS_IN, false);
}

void
TermWidget::focusOutEvent(QFocusEvent *event)
{
    m_focused = m_dragging = false;
    refocus();
    m_term->keymap()->reset();

    // Cancel the resize effect
    m_effects->cancelResizeEffect(m_scrollport);

    if (m_focusEvents)
        m_term->pushInput(m_stack->manager(), TSQ_FOCUS_OUT, false);
}

void
TermWidget::showEvent(QShowEvent *event)
{
    m_visible = true;

    if (m_resized) {
        m_resized = false;
        updateSize(size());
    }
    else if (m_badgeRunning)
        m_badgeSag->start();

    m_scrollport->setShown();
    redisplay();
}

void
TermWidget::hideEvent(QHideEvent *event)
{
    m_visible = false;
    m_scrollport->setHidden();

    if (m_badgeRunning)
        m_badgeSag->stop();
}

void
TermWidget::wheelEvent(QWheelEvent *e)
{
    if ((!m_altScroll && !m_mouseReport) || e->modifiers() & Qt::ShiftModifier) {
        m_scroll->forwardWheelEvent(e);
        return;
    }

    int y = e->angleDelta().y();
    if (y == 0)
        return;

    if (m_altScroll) {
        int count = (e->modifiers() & Qt::ControlModifier) ?
            m_scrollport->height() - 1 :
            5;
        bool appcur = m_term->flags() & Tsq::AppCuKeys;
        QString seq;

        if (y < 0) {
            seq = appcur ? TSQ_APPCURSOR_DOWN : TSQ_CURSOR_DOWN;
        } else {
            seq = appcur ? TSQ_APPCURSOR_UP : TSQ_CURSOR_UP;
        }

        m_term->pushInput(m_stack->manager(), seq.repeated(count), false);
    }
    else {
        QPoint p = screenPosition(e->x(), e->y());

        if (p.x() >= 0) {
            Tsq::MouseEventFlags flags = mouseWheelFlags(e, y < 0);
            m_term->mouseEvent(flags, p);
        }
    }
}

void
TermWidget::resizeEvent(QResizeEvent *event)
{
    if (m_visible)
        updateSize(event->size());
    else
        m_resized = true;

    m_sel->resize(event->size());
}

QSize
TermWidget::sizeHint() const
{
    const QSize &size = m_term->screen()->size();
    int w = qCeil(size.width() * m_cellSize.width());
    int h = qCeil(size.height() * m_cellSize.height());
    return QSize(w, h);
}

QSize
TermWidget::minimumSizeHint() const
{
    return QSize();
}

void
TermWidget::setFocusFade(int focusFade)
{
    m_focusFade = focusFade;
    update();
    parentWidget()->update();
}

void
TermWidget::initialize()
{
    SelectionWidget::initialize();

    // Command icon
    {
        QPainterPath bolt;
        QVector<QPointF> boltPoly = { QPointF(60.0, 10.0),
                                      QPointF(56.0, 36.0),
                                      QPointF(74.0, 34.0),
                                      QPointF(37.0, 70.0),
                                      QPointF(42.0, 44.0),
                                      QPointF(23.0, 46.0) };
        bolt.addPolygon(boltPoly);
        bolt.closeSubpath();

        s_commandPath.addEllipse(10.0, 0.0, 80.0, 80.0);
        s_commandPath -= bolt;
    }

    // Selection icon
    QVector<QPointF> leftPoly = { QPointF(15.0, 5.0),
                                  QPointF(45.0, 5.0),
                                  QPointF(45.0, 25.0),
                                  QPointF(30.0, 25.0),
                                  QPointF(30.0, 75.0),
                                  QPointF(15.0, 75.0) };

    QVector<QPointF> rightPoly = { QPointF(70.0, 5.0),
                                   QPointF(85.0, 5.0),
                                   QPointF(85.0, 75.0),
                                   QPointF(55.0, 75.0),
                                   QPointF(55.0, 55.0),
                                   QPointF(70.0, 55.0) };

    s_selectPath.addPolygon(leftPoly);
    s_selectPath.closeSubpath();
    s_selectPath.addPolygon(rightPoly);
    s_selectPath.closeSubpath();

    // Peer icon
    leftPoly = { QPointF(5.0, 23.0),
                 QPointF(65.0, 23.0),
                 QPointF(65.0, 5.0),
                 QPointF(95.0, 32.0),
                 QPointF(65.0, 59.0),
                 QPointF(65.0, 41.0),
                 QPointF(5.0, 41.0) };
    rightPoly = { QPointF(95.0, 77.0),
                  QPointF(35.0, 77.0),
                  QPointF(35.0, 95.0),
                  QPointF(5.0, 68.0),
                  QPointF(35.0, 41.0),
                  QPointF(35.0, 59.0),
                  QPointF(95.0, 59.0) };
    s_peerPath.addPolygon(leftPoly);
    s_peerPath.closeSubpath();
    s_peerPath.addPolygon(rightPoly);
    s_peerPath.closeSubpath();

    // Ownership icon
    QRectF torsoRect(0.0, 5.0, 50.0, 70.0);

    s_ownerPath.addEllipse(10.0, -30.0, 30.0, 30.0);
    s_ownerPath.moveTo(0.0, 40.0);
    s_ownerPath.arcTo(torsoRect, 180.0, -180.0);
    s_ownerPath.closeSubpath();

    s_ownerPath.translate(35.0, 30.0);

    // Lock icon
    QRectF outerRect(5.0, -20.0, 40.0, 40.0);
    QRectF innerRect(15.0, -10.0, 20.0, 20.0);

    s_lockPath.addRect(0.0, 10.0, 50.0, 40.0);
    s_lockPath.addRect(5.0, 0.0, 10.0, 10.0);
    s_lockPath.addRect(35.0, 0.0, 10.0, 10.0);
    s_lockPath.moveTo(5.0, 0.0);
    s_lockPath.arcTo(outerRect, 180.0, -180.0);
    s_lockPath.moveTo(35.0, 0.0);
    s_lockPath.arcTo(innerRect, 0.0, 180.0);
    s_lockPath.closeSubpath();

    s_lockPath.translate(15.0, 20.0);

    // Alert icon
    leftPoly = { QPointF(62.0, 70.0),
                 QPointF(70.0, 62.0),
                 QPointF(40.0, 32.0),
                 QPointF(32.0, 40.0) };
    {
        QPainterPath inner;
        inner.addEllipse(12.0, 12.0, 26.0, 26.0);
        QPainterPath handle;
        handle.addPolygon(leftPoly);
        handle.closeSubpath();
        s_alertPath.addEllipse(0.0, 0.0, 50.0, 50.0);
        s_alertPath += handle;
        s_alertPath -= inner;
    }
    s_alertPath.translate(15.0, 0.0);

    // Leader icon
    leftPoly = { QPointF(0.0, 0.0),
                 QPointF(15.0, 10.0),
                 QPointF(30.0, 0.0),
                 QPointF(45.0, 10.0),
                 QPointF(60.0, 0.0),
                 QPointF(50.0, 70.0),
                 QPointF(10.0, 70.0) };
    rightPoly = { QPointF(30.0, 15.0),
                  QPointF(45.0, 30.0),
                  QPointF(35.0, 30.0),
                  QPointF(35.0, 60.0),
                  QPointF(25.0, 60.0),
                  QPointF(25.0, 30.0),
                  QPointF(15.0, 30.0) };
    s_leaderPath.addPolygon(leftPoly);
    s_leaderPath.closeSubpath();
    {
        QPainterPath arrow;
        arrow.addPolygon(rightPoly);
        arrow.closeSubpath();
        s_leaderPath -= arrow;
    }
    s_leaderPath.translate(15.0, 0.0);

    // Follower icon
    rightPoly = { QPointF(30.0, 5.0),
                  QPointF(45.0, 20.0),
                  QPointF(35.0, 20.0),
                  QPointF(35.0, 60.0),
                  QPointF(25.0, 60.0),
                  QPointF(25.0, 20.0),
                  QPointF(15.0, 20.0) };

    {
        QPainterPath arrow;
        arrow.addPolygon(rightPoly);
        arrow.closeSubpath();
        s_followPath += arrow;
        arrow.translate(-30.0, 10.0);
        s_followPath += arrow;
        arrow.translate(60.0, 0.0);
        s_followPath += arrow;
    }
    s_followPath.translate(15.0, 0.0);
}
