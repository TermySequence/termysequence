// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "selwidget.h"
#include "termwidget.h"
#include "selection.h"
#include "scrollport.h"
#include "buffers.h"
#include "term.h"
#include "settings/global.h"

#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QtMath>

static QPainterPath s_path;

//
// Main widget
//
SelectionWidget::SelectionWidget(TermScrollport *scrollport, TermWidget *parent) :
    QWidget(parent),
    m_parent(parent),
    m_scrollport(scrollport),
    m_buffers(scrollport->buffers()),
    m_sel(m_buffers->selection()),
    m_dashPattern(2)
{
    setMouseTracking(true);
    setContextMenuPolicy(Qt::NoContextMenu);
    setVisible(false);

    m_upper = new SelectionHandle(false, this);
    m_lower = new SelectionHandle(true, this);

    connect(m_sel, SIGNAL(activated()), SLOT(handleStart()));
    connect(m_sel, SIGNAL(restarted()), SLOT(handleRestart()));
    connect(m_sel, SIGNAL(deactivated()), SLOT(handleStop()));
    connect(m_sel, SIGNAL(modified()), SLOT(handleModify()));
    m_mocHandle = connect(m_sel, &Selection::activeHandleChanged,
                          this, &SelectionWidget::activateHandle);

    connect(scrollport, SIGNAL(selectRequest(int,int)), SLOT(handleSelectRequest(int,int)));
    connect(scrollport->term(), SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(update()));
    connect(scrollport->term(), SIGNAL(paletteChanged()), SLOT(recolor()));

    connect(g_global, &GlobalSettings::blinkTimeChanged,
            this, &SelectionWidget::calculateDashParams);

    recolor();
}

void
SelectionWidget::startAnimation()
{
    m_dashCount = m_dashSize * g_global->cursorBlinks();

    if (m_dashCount && !m_dashId)
        m_dashId = startTimer(m_dashTime);
}

void
SelectionWidget::stopAnimation()
{
    if (m_dashId) {
        killTimer(m_dashId);
        m_dashId = 0;
    }
}

void
SelectionWidget::activateHandle(bool upper)
{
    if (m_activeHandle != upper) {
        m_activeHandle = upper;
        SelectionHandle *arr[2] = { m_lower, m_upper };
        arr[upper]->setActive(true);
        arr[!upper]->setActive(false);

        // Avoid reentering
        disconnect(m_mocHandle);
        emit m_sel->activeHandleChanged(upper);
        m_mocHandle = connect(m_sel, &Selection::activeHandleChanged,
                              this, &SelectionWidget::activateHandle);
    }
}

void
SelectionWidget::activateHandleArg(int arg, bool defval)
{
    bool nextval;

    switch (arg) {
    case 1:
        nextval = true;
        break;
    case 2:
        nextval = false;
        break;
    default:
        nextval = (m_activeHandle != -1) ? m_activeHandle : defval;
    }

    activateHandle(nextval);
}

inline void
SelectionWidget::switchHandleArg(int arg)
{
    bool nextval;

    switch (arg) {
        case 1:
            nextval = true;
            break;
        case 2:
            nextval = false;
            break;
        default:
            nextval = !m_activeHandle;
    }

    activateHandle(nextval);
}

void
SelectionWidget::handleSelectRequest(int type, int arg)
{
    std::pair<index_t,bool> rc;

    switch (type) {
    case SELECTREQ_HANDLE:
        switchHandleArg(arg);
        rc.first = m_activeHandle ? m_sel->startRow : m_sel->endRow;
        rc.second = true;
        break;
    case SELECTREQ_FORWARDCHAR:
        activateHandleArg(arg, false);
        rc = m_sel->forwardChar(m_activeHandle);
        break;
    case SELECTREQ_BACKCHAR:
        activateHandleArg(arg, true);
        rc = m_sel->backChar(m_activeHandle);
        break;
    case SELECTREQ_FORWARDWORD:
        activateHandleArg(arg, false);
        rc = m_sel->forwardWord(m_activeHandle);
        break;
    case SELECTREQ_BACKWORD:
        activateHandleArg(arg, true);
        rc = m_sel->backWord(m_activeHandle);
        break;
    case SELECTREQ_DOWNLINE:
        activateHandleArg(arg, false);
        rc = m_sel->forwardLine(m_activeHandle);
        break;
    case SELECTREQ_UPLINE:
        activateHandleArg(arg, true);
        rc = m_sel->backLine(m_activeHandle);
        break;
    default:
        return;
    }

    m_scrollport->scrollToRow(rc.first, false);
    if (!rc.second)
        (m_activeHandle ? m_upper : m_lower)->startBounce();
}

void
SelectionWidget::handleStart()
{
    if (!m_mocOffset)
        m_mocOffset = connect(m_scrollport, SIGNAL(offsetChanged(int)),
                              SLOT(handleOffsetChanged()));
    handleOffsetChanged();
    show();
}

void
SelectionWidget::handleRestart()
{
    // Stop doing stuff
    m_active = false;
    m_upper->setVisible(false);
    m_lower->setVisible(false);
    disconnect(m_mocOffset);
    stopAnimation();
    m_scrollport->setSelecting(false);
}

void
SelectionWidget::handleStop()
{
    m_active = false;
    setVisible(false);
    disconnect(m_mocOffset);
    stopAnimation();
    m_scrollport->setSelecting(false);
}

void
SelectionWidget::calculatePosition()
{
    unsigned startX = m_buffers->xByPos(m_sel->startRow, m_sel->startCol);
    int ys = m_sel->startRow - m_buffers->origin() - m_scrollport->offset();
    m_upper->move(startX * m_cw - m_cw / 2, ys * m_ch - m_cw / 2);

    unsigned endX = m_buffers->xByPos(m_sel->endRow, m_sel->endCol);
    int ye = m_sel->endRow - m_buffers->origin() - m_scrollport->offset();
    m_lower->move(endX * m_cw - m_cw / 2, ye * m_ch);

    qreal y, t;
    unsigned x;
    m_lines.clear();

    if (m_sel->startRow == m_sel->endRow) {
        // Top line
        y = ys * m_ch + m_half;
        m_lines.append(QPointF(startX * m_cw, y));
        m_lines.append(QPointF(endX * m_cw, y));

        // Bottom line
        y = (ye + 1) * m_ch - m_half;
        m_lines.append(QPointF(endX * m_cw, y));
        m_lines.append(QPointF(startX * m_cw, y));
    } else {
        // Top line
        y = ys * m_ch + m_half;
        x = m_buffers->xSize(m_sel->startRow);
        m_lines.append(QPointF(startX * m_cw, y));
        m_lines.append(QPointF(x * m_cw, y));

        // Top start to handle
        if (startX) {
            y += m_ch;
            m_lines.append(QPointF(0, y));

            x = qMin(startX, m_buffers->xSize(m_sel->startRow + 1));
            if (m_sel->startRow + 1 == m_sel->endRow)
                x = qMin(x, endX);
            m_lines.append(QPointF(x * m_cw, y));
        }

        // Bottom line
        y = (ye + 1) * m_ch - m_half;
        m_lines.append(QPointF(endX * m_cw, y));
        m_lines.append(QPointF(0, y));

        // Bottom handle to end
        x = m_buffers->xSize(m_sel->endRow - 1);
        if (x > endX) {
            y -= m_ch;
            unsigned x2 = endX;
            if (m_sel->startRow + 1 == m_sel->endRow)
                x2 = qMax(x2, startX);
            if (x2 < x) {
                m_lines.append(QPointF(x * m_cw, y));
                m_lines.append(QPointF(x2 * m_cw, y));
            }
        }

        // Left line
        m_lines.append(QPointF(m_half, (ye + 1) * m_ch));
        m_lines.append(QPointF(m_half, (ys + 1) * m_ch));

        index_t row = m_buffers->origin() + m_scrollport->offset();
        index_t start = qMax(m_sel->startRow, row);
        index_t end = qMin(m_sel->endRow, row + m_scrollport->height());

        ys = start - m_buffers->origin() - m_scrollport->offset();
        y = ys * m_ch;

        for (row = start; row < end; ++row, ++ys) {
            // Right lines
            x = m_buffers->xSize(row);
            t = qCeil(x * m_cw) - m_half;
            m_lines.append(QPointF(t, y));
            m_lines.append(QPointF(t, y += m_ch));

            unsigned x2 = m_buffers->xSize(row + 1);

            if (ys == m_scrollport->height() - 1)
                break;
            if (row == m_sel->endRow - 1 && (x2 = qMin(x2, endX)) < x)
                break;
            if (row == m_sel->startRow)
                x2 = qMax(x2, startX);

            // Inner lines
            if (x < x2) {
                m_lines.append(QPointF(x * m_cw, y + m_half));
                m_lines.append(QPointF(x2 * m_cw, y + m_half));
            } else if (x > x2) {
                m_lines.append(QPointF(x * m_cw, y - m_half));
                m_lines.append(QPointF(x2 * m_cw, y - m_half));
            }
        }
    }
}

void
SelectionWidget::handleModify()
{
    if (m_active) {
        calculatePosition();
        startAnimation();
        update();
    }
}

void
SelectionWidget::handleOffsetChanged()
{
    index_t first = m_buffers->origin() + m_scrollport->offset();
    index_t last = first + m_scrollport->height();

    if ((m_active = last > m_sel->startRow && first <= m_sel->endRow)) {
        calculatePosition();
        m_upper->show();
        m_lower->show();
        startAnimation();
        raise();
    } else {
        m_upper->setVisible(false);
        m_lower->setVisible(false);
        stopAnimation();
        lower();
    }

    m_scrollport->setSelecting(m_active);
    update();
}

void
SelectionWidget::calculateDashParams(unsigned blinkTime)
{
    // make blinkTime divisible by 20
    int bt = (blinkTime / 20) * 20;

    m_dashSize = 2 * qCeil(m_cw / 2);
    m_dashSize += !m_dashSize * 2;
    m_dashSize = qMin(m_dashSize, bt / 10);

    m_dashTime = bt * 2 / m_dashSize;
    m_dashPattern[0] = m_dashPattern[1] = m_dashSize / 2;

    if (m_mocOffset)
        handleOffsetChanged();
}

void
SelectionWidget::calculateCellSize()
{
    QSizeF cellSize = m_parent->cellSize();
    m_cw = cellSize.width();
    m_ch = cellSize.height();

    m_line = 1 + (int)m_ch / CURSOR_BOX_INCREMENT;
    m_half = m_line / 2.0;

    QSize dim(qCeil(m_cw), m_ch + m_cw / 2);
    m_upper->resize(dim);
    m_lower->resize(dim);

    calculateDashParams(g_global->blinkTime());
}

void
SelectionWidget::paintEvent(QPaintEvent *event)
{
    if (m_active) {
        QPainter painter(this);
        QPen pen(painter.pen());
        pen.setColor(m_scrollport->term()->bg());
        pen.setWidth(m_line);
        painter.setPen(pen);
        painter.drawLines(m_lines);

        pen.setDashPattern(m_dashPattern);
        pen.setDashOffset(m_dashOffset);
        pen.setColor(m_scrollport->term()->fg());
        painter.setPen(pen);
        painter.drawLines(m_lines);
    }
}

void
SelectionWidget::timerEvent(QTimerEvent *)
{
    m_dashOffset += m_dashOffset ? -1 : m_dashSize - 1;
    update();

    if (--m_dashCount == 0) {
        killTimer(m_dashId);
        m_dashId = 0;
    }
}

void
SelectionWidget::recolor()
{
    const auto &palette = m_scrollport->term()->palette();
    m_handleColors[0] = palette[PALETTE_APP_HANDLE_OFF];
    m_handleColors[1] = palette[PALETTE_APP_HANDLE_ON];
    m_upper->update();
    m_lower->update();
}

//
// Handle
//
SelectionHandle::SelectionHandle(bool lower, SelectionWidget *parent) :
    QWidget(parent),
    m_parent(parent),
    m_lower(lower)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setCursor(Qt::SizeFDiagCursor);
    setVisible(false);

    m_bounceAnim = new QPropertyAnimation(this, B("bounce"), this);
    QEasingCurve ec(QEasingCurve::OutElastic);
    ec.setAmplitude(2.0);
    m_bounceAnim->setEasingCurve(ec);
    m_bounceAnim->setDuration(BLINK_TIME * 2);
    m_bounceAnim->setEndValue(0);
}

void
SelectionHandle::setActive(bool active)
{
    m_active = active;
    update();
}

void
SelectionHandle::move(int x, int y)
{
    if (m_bounceAnim->state() != QAbstractAnimation::Stopped)
        m_bounceAnim->stop();

    m_bounce = 0;
    m_realX = x;
    QWidget::move(x, y);
}

void
SelectionHandle::setBounce(int bounce)
{
    m_bounce = bounce;

    QPoint p = pos();
    p.rx() = m_realX + bounce;
    QWidget::move(p);
}

void
SelectionHandle::startBounce()
{
    if (m_bounceAnim->state() != QAbstractAnimation::Running) {
        m_bounce = m_parent->m_cw / (m_lower ? 2 : -2);
        m_bounceAnim->start();
    }
}

void
SelectionHandle::paintEvent(QPaintEvent *event)
{
    qreal scalex = m_parent->m_cw / 100, scaley = m_parent->m_ch / 100;
    QRect rect(0, 0, qCeil(m_parent->m_cw), qCeil(m_parent->m_cw * 0.65));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_lower) {
        painter.translate(width(), height());
        painter.rotate(180.0);
    }

    painter.fillRect(rect, m_parent->m_handleColors[m_active]);
    painter.translate(0, m_parent->m_cw / 2);
    painter.scale(scalex, scaley);
    painter.fillPath(s_path, m_parent->m_handleColors[m_active]);
}

void
SelectionHandle::paintPreview(QPainter *painter, const TermPalette *palette,
                              const QSizeF &cellSize, bool active)
{
    qreal cw = cellSize.width(), ch = cellSize.height();
    qreal scalex = cw / 100, scaley = ch / 100;
    QRect rect(0, 0, qCeil(cw), qCeil(cw * 0.65));

    if (active) {
        painter->translate(qCeil(cw), qCeil(ch + cw / 2));
        painter->rotate(180.0);
    }

    QColor color = palette->at(PALETTE_APP_HANDLE_OFF + active);
    painter->fillRect(rect, color);
    painter->translate(0, cw / 2);
    painter->scale(scalex, scaley);
    painter->fillPath(s_path, color);
}

void
SelectionHandle::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_parent->m_sel->moveAnchor(m_lower);
        m_parent->m_parent->startSelecting(
            mapTo(m_parent->m_parent, event->pos()));
    }
}

void
SelectionHandle::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        m_parent->m_parent->finishSelecting(
            mapTo(m_parent->m_parent, event->pos()));
    }
}

void
SelectionHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_selecting)
        m_parent->m_parent->moveSelecting(
            mapTo(m_parent->m_parent, event->pos()));
}

void
SelectionWidget::initialize()
{
    // Corner
    QVector<QPointF> poly = { QPointF(0.0, 0.0),
                              QPointF(100.0, 0.0),
                              QPointF(100.0, 15.0),
                              QPointF(65.0, 15.0),
                              QPointF(65.0, 100.0),
                              QPointF(0.0, 100.0) };
    s_path.addPolygon(poly);
    s_path.closeSubpath();
}
