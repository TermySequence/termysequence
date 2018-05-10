// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "thumbwidget.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "buffers.h"
#include "screen.h"
#include "thumbport.h"
#include "blinktimer.h"
#include "effecttimer.h"
#include "contentmodel.h"
#include "highlight.h"
#include "settings/global.h"
#include "settings/profile.h"

#include <QPropertyAnimation>
#include <QGuiApplication>

ThumbWidget::ThumbWidget(TermInstance *term, TermManager *manager, QWidget *parent) :
    QWidget(parent),
    DisplayIterator(term),
    m_manager(manager),
    m_thumbport(new TermThumbport(m_term, this)),
    m_blink(g_listener->blink()),
    m_effects(g_listener->effects())
{
    m_resizeAnim = new QPropertyAnimation(this, B("resizeFade"), this);
    m_resizeAnim->setDuration(BLINK_TIME);
    m_resizeAnim->setEndValue(0);
    m_flashAnim = new QPropertyAnimation(this, B("flashFade"), this);
    m_flashAnim->setEasingCurve(QEasingCurve::OutQuint);
    m_flashAnim->setDuration(BLINK_TIME);
    m_flashAnim->setStartValue(255);
    m_flashAnim->setEndValue(0);

    m_blink->addViewport(m_thumbport);
    m_effects->addViewport(m_thumbport);

    connect(m_term->buffers(), SIGNAL(contentChanged()), SLOT(redisplay()));
    connect(m_term, SIGNAL(sizeChanged(QSize)), SLOT(handleSizeChanged(QSize)));
    connect(m_blink, SIGNAL(timeout()), SLOT(blink()));
    connect(m_thumbport, SIGNAL(resizeEffectChanged(int)), SLOT(handleResizeEffectChanged(int)));
    connect(m_manager, SIGNAL(activeChanged(bool)), SLOT(refocus()));
    connect(m_manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)), SLOT(refocus()));
    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));
    connect(m_term, SIGNAL(paletteChanged()), SLOT(update()));
    connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_term, SIGNAL(bellRang()), SLOT(bell()));
    connect(m_term, SIGNAL(alertChanged()), SLOT(handleAlert()));
    connect(m_term, SIGNAL(processExited(QRgb)), SLOT(handleProcessExited(QRgb)));
    connect(m_term, SIGNAL(cursorHighlight()), SLOT(handleCursorHighlight()));
    connect(g_global, SIGNAL(thumbIndexChanged(bool)), SLOT(handleShowIndexChanged(bool)));
    connect(m_manager, SIGNAL(menuFinished()), SLOT(startBlinking()));

    refont(m_term->font());
    recolor(m_term->bg(), m_term->fg());
    handleShowIndexChanged(g_global->thumbIndex());
    refocus();
    redisplay();
    handleAlert();
}

void
ThumbWidget::blink()
{
    if (m_thumbport->updateBlink)
        update();
}

void
ThumbWidget::redisplay()
{
    if (!m_term->updating() && m_visible)
    {
        calculateCells(m_thumbport, false);
        m_blink->setBlinkEffect(m_thumbport);
        update();
    }
}

void
ThumbWidget::refocus()
{
    bool focused = m_manager->active();
    bool primary = focused && m_manager->activeTerm() == m_term;

    m_thumbport->setPrimary(primary);
    m_thumbport->setFocused(focused);

    m_blink->setBlinkEffect(m_thumbport);
    update();
}

void
ThumbWidget::recolor(QRgb bg, QRgb fg)
{
    m_bg = bg;
    m_fg = fg;
    m_flash.setRgb(fg);
    m_flash.setAlpha(m_flashFade);
    update();
}

void
ThumbWidget::startBlinking()
{
    m_blink->setBlinkEffect(m_thumbport);
}

void
ThumbWidget::updateIndexBounds()
{
    qreal w = width();
    qreal h = height() / 2.0;
    qreal cw = m_cellSize.width();
    qreal ch = m_cellSize.height();

    m_indexScale = h / ch;

    int s = m_indexStr.size();
    qreal x = (w / m_indexScale - s * cw) / 2.0;

    m_indexRect.setRect(x, 0.0, s * cw, ch);
}

void
ThumbWidget::updateEffects(const QSize &emulatorSize)
{
    int w = width();
    int h = height();

    m_resizeStr = L("%1x%2").arg(emulatorSize.width()).arg(emulatorSize.height());

    int cw = m_cellSize.width() / 2;
    cw += (cw & 1);
    int ch = m_cellSize.height();
    ch += (ch & 1);

    int rw = m_metrics.width(m_resizeStr) + cw;

    m_resizeRect.setRect((w - rw) / 2, (h - ch) / 2, rw, ch);
}

void
ThumbWidget::handleSizeChanged(QSize emulatorSize)
{
    rescale();
    updateEffects(emulatorSize);
    updateGeometry();

    // Activate the resize effect
    m_effects->setResizeEffect(m_thumbport);
}

void
ThumbWidget::reindex()
{
    m_indexStr = m_term->attributes().value(g_attr_TSQT_INDEX);
    updateIndexBounds();
    update();
}

void
ThumbWidget::handleShowIndexChanged(bool showIndex)
{
    if (showIndex) {
        connect(m_term, SIGNAL(stacksChanged()), SLOT(reindex()), Qt::UniqueConnection);
        reindex();
    }
    else {
        disconnect(m_term, SIGNAL(stacksChanged()), this, SLOT(reindex()));
        m_indexStr.clear();
        update();
    }
}

void
ThumbWidget::handleResizeEffectChanged(int resizeEffect)
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
ThumbWidget::refont(const QFont &font)
{
    setDisplayFont(font);

    rescale();
    updateEffects(m_term->screen()->size());
    updateGeometry();
    update();
}

void
ThumbWidget::rescale()
{
    qreal a = width();
    qreal b = m_term->screen()->width() * m_cellSize.width();

    if (b < 1.0)
        b = 1.0;

    m_thumbScale = a / b;
    updateIndexBounds();
}

void
ThumbWidget::bell()
{
    if (g_global->thumbBell() == BellFlash &&
        m_flashAnim->state() != QAbstractAnimation::Running)
    {
        m_flash = m_fg;
        m_flashAnim->setLoopCount(1);
        m_flashAnim->start();
    }
}

void
ThumbWidget::handleAlert()
{
    short flashes = m_term->alertFlashes();
    if (flashes) {
        m_flash = m_fg;
        m_flashAnim->setLoopCount(flashes);
        m_flashAnim->start();
    } else {
        m_flashAnim->setLoopCount(1);
    }
}

void
ThumbWidget::handleProcessExited(QRgb exitColor)
{
    auto *manager = g_listener->activeManager();
    bool primary = manager && manager->activeTerm() == m_term;

    if (m_term->profile()->exitEffect() + !primary > 1) {
        m_flash = exitColor;
        m_flashAnim->setLoopCount(1);
        m_flashAnim->start();
    }
}

void
ThumbWidget::handleCursorHighlight()
{
    new CursorHighlight(this, this, m_thumbport);
}

void
ThumbWidget::paintImage(QPainter &painter, const Region *region) const
{
    QString id = region->attributes.value(g_attr_CONTENT_ID);
    QPixmap pixmap = m_term->content()->pixmap(id);
    int height = (region->endRow - region->startRow + 1) * m_cellSize.height();
    int offset = region->startRow - m_term->buffers()->origin();

    QRect bounds(region->startCol * m_cellSize.width(),
        (offset - m_thumbport->offset()) * m_cellSize.height(),
        (region->endCol - region->startCol) * m_cellSize.width(),
        height);

    if (region->attributes.value(g_attr_CONTENT_ASPECT) != A("0")) {
        QSize size(pixmap.size());
        size.scale(bounds.size(), Qt::KeepAspectRatio);
        bounds.setSize(size);
        if (height > bounds.height())
            bounds.translate(0, (height - bounds.height()) / 2);
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(bounds, pixmap);
}

void
ThumbWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_bg);
    CellState state(m_thumbport->textBlink ? Tsq::Blink|Tsq::Invisible : Tsq::Invisible);

    QColor fg = state.fg = m_term->fg();
    state.bg = m_term->bg();
    state.scale = m_thumbScale;

    painter.scale(m_thumbScale, m_thumbScale);
    painter.setFont(m_font);

    // Draw image regions
    for (const RegionRef &region: m_thumbport->regions().list)
        if (region.ptr->type() == Tsqt::RegionImage)
            paintImage(painter, static_cast<const Region*>(region.ptr));

    // Draw emulator contents
    for (const DisplayCell &cell: m_displayCells) {
        paintThumb(painter, cell, state);
    }

    // Draw cursor
    if (!m_thumbport->cursorBlink && !m_term->overlayActive())
    {
        DisplayCell i(m_cursorCell);
        i.flags ^= Tsq::Inverse;
        paintThumb(painter, i, state);
    }

    // Draw stack index background
    if (!m_indexStr.isEmpty()) {
        QFont font = painter.font();
        font.setBold(false);
        font.setUnderline(false);
        painter.setFont(font);

        QColor indexColor = fg;
        indexColor.setAlphaF(0.5);
        painter.resetTransform();
        painter.setPen(indexColor);
        painter.scale(m_indexScale, m_indexScale);
        painter.fillRect(m_indexRect, QColor(state.bg));
        painter.drawText(m_indexRect, Qt::AlignCenter, m_indexStr);
    }

    // Draw flash effect
    if (m_flashFade) {
        painter.resetTransform();
        painter.fillRect(rect(), m_flash);
    }

    // Draw resize popup
    if (m_thumbport->resizeEffect()) {
        painter.resetTransform();
        QFont font = painter.font();
        font.setWeight(m_fontWeight);
        font.setUnderline(false);
        painter.setFont(font);

        QPalette pal = QGuiApplication::palette();
        QColor color = pal.color(QPalette::Window);
        color.setAlpha(m_resizeFade);
        painter.fillRect(m_resizeRect, color);

        color = pal.color(QPalette::WindowText);
        color.setAlpha(m_resizeFade);
        painter.setPen(color);
        painter.drawText(m_resizeRect, Qt::AlignCenter, m_resizeStr);

        color = pal.color(QPalette::Dark);
        color.setAlpha(m_resizeFade);
        painter.setPen(color);
        painter.drawRect(m_resizeRect);
    }
}

void
ThumbWidget::resizeEvent(QResizeEvent *event)
{
    rescale();
    updateEffects(m_term->screen()->size());
}

void
ThumbWidget::showEvent(QShowEvent *event)
{
    m_visible = true;
    redisplay();
}

void
ThumbWidget::hideEvent(QHideEvent *event)
{
    m_visible = false;
}

int
ThumbWidget::heightForWidth(int w) const
{
    const QSize &size = m_term->screen()->size();
    return (w * size.height() * m_cellSize.height()) / (size.width() * m_cellSize.width());
}

int
ThumbWidget::widthForHeight(int h) const
{
    const QSize &size = m_term->screen()->size();
    return (h * size.width() * m_cellSize.width()) / (size.height() * m_cellSize.height());
}

void
ThumbWidget::setResizeFade(int resizeFade)
{
    m_resizeFade = resizeFade;
    update();
}

void
ThumbWidget::setFlashFade(int flashFade)
{
    m_flashFade = flashFade;
    m_flash.setAlpha(m_flashFade);
    update();
}
