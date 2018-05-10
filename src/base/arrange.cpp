// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "arrange.h"
#include "stack.h"
#include "term.h"
#include "termwidget.h"
#include "scroll.h"
#include "scrollport.h"
#include "marks.h"
#include "minimap.h"
#include "modtimes.h"
#include "settings/profile.h"
#include "settings/termlayout.h"

#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QClipboard>
#include <QResizeEvent>

//
// Main widget
//
TermArrange::TermArrange(TermStack *stack, TermInstance *term) :
    m_term(term)
{
    m_scrollport = new TermScrollport(stack->manager(), term, this);

    TermScroll *scroll = new TermScroll(m_scrollport, this);
    m_widget = new TermWidget(stack, m_scrollport, scroll, this);
    TermMarks *marks = new TermMarks(m_scrollport, this);
    TermMinimap *minimap = new TermMinimap(m_scrollport, this);
    m_modtimes = new TermModtimes(m_scrollport, this);

    m_widgets[LAYOUT_WIDGET_TERM] = m_widget;
    m_widgets[LAYOUT_WIDGET_SCROLL] = scroll;
    m_widgets[LAYOUT_WIDGET_MARKS] = marks;
    m_widgets[LAYOUT_WIDGET_MINIMAP] = minimap;
    m_widgets[LAYOUT_WIDGET_MODTIME] = m_modtimes;

    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_widget);

    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(rebackground(QRgb)));
    connect(m_term, SIGNAL(layoutChanged(const QString&)), SLOT(handleLayoutChanged(const QString&)));

    rebackground(m_term->bg());
    m_layout.parseLayout(m_term->layout());

    calculateSizeHint();
}

void
TermArrange::rebackground(QRgb bg)
{
    setStyleSheet(L("TermArrange {background-color:#%1}").arg(bg, 6, 16, Ch0));
}

void
TermArrange::handleLayoutChanged(const QString &layoutStr)
{
    m_layout.parseLayout(layoutStr);
    relayout(size());
}

void
TermArrange::calculateSizeHint()
{
    int margin = 1 + m_widget->cellSize().height() / MARGIN_INCREMENT;

    m_sizeHint = m_widget->sizeHint();
    m_sizeHint.rheight() += 2 * m_layout.itemMargin(0, margin);
    m_sizeHint.rwidth() += m_layout.itemExtraWidth(0, margin);

    for (int i = 1; i < LAYOUT_N_WIDGETS; ++i) {
        if (m_layout.itemEnabled(i)) {
            m_sizeHint.rwidth() += m_widgets[i]->sizeHint().width();
            m_sizeHint.rwidth() += m_layout.itemExtraWidth(i, margin);
        }
    }
}

void
TermArrange::relayout(const QSize &size)
{
    QWidget *order[ARRANGE_N_WIDGETS] = { 0 };
    QRect bounds[ARRANGE_N_WIDGETS];

    // Calculate widget sizes
    int margin = 1 + m_widget->cellSize().height() / MARGIN_INCREMENT;
    int termWidth = size.width();
    int minHeight = 0;
    int pos = 0;
    int index = 0;
    int termIndex = 0;
    int m, w, h, i;

    for (i = 0; i < LAYOUT_N_WIDGETS; ++i) {
        int item = m_layout.itemAt(i);

        if (!m_layout.enabledAt(i)) {
            m_widgets[item]->hide();
            item += LAYOUT_N_WIDGETS;
            if (m_widgets[item])
                m_widgets[item]->hide();
            continue;
        }

        m = m_layout.itemMargin(item, margin);
        w = m_widgets[item]->sizeHint().width();
        h = size.height() - 2 * m;

        order[index] = m_widgets[item];
        bounds[index].setRect(pos + m, m, w, h);

        if (item == 0) {
            termIndex = index;
            pos += 2 * m;
            termWidth -= 2 * m;
        } else {
            w += 2 * m;
            pos += w;
            termWidth -= w;
        }

        h = m_widgets[item]->minimumSizeHint().height();
        minHeight = qMax(minHeight, h + 2 * m);

        ++index;
        item += LAYOUT_N_WIDGETS;

        if (m_layout.separatorAt(i)) {
            if (!m_widgets[item])
                m_widgets[item] = new TermSeparator(m_term, this);

            order[index] = m_widgets[item];
            bounds[index++].setRect(pos, 0, margin, size.height());

            pos += margin;
            termWidth -= margin;
        }
        else if (m_widgets[item]) {
            m_widgets[item]->hide();
        }
    }

    // See if there is enough room
    if (termWidth < 0 || size.height() < minHeight) {
        for (i = 0; i < index; ++i)
            order[i]->hide();

        return;
    }

    // Adjust widgets to the right of term
    bounds[termIndex].setWidth(termWidth);
    for (i = termIndex + 1; i < index; ++i) {
        bounds[i].translate(termWidth, 0);
    }

    // Set widget geometries
    for (i = 0; i < index; ++i) {
        order[i]->setGeometry(bounds[i]);
        order[i]->show();
    }

    const auto &termBounds = bounds[termIndex];
    m_widget->updateViewportSize(termBounds.size());

    // Calculate focus fade effect bounds
    m_focusFadeBounds.setRect(termBounds.x() - margin / 2.0,
                              termBounds.y() - margin / 2.0,
                              termBounds.width() + margin,
                              termBounds.height() + margin);
    m_margin = margin;
}

void
TermArrange::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // Draw focus effect
    if (m_scrollport->focusEffect())
    {
        QColor color(m_term->fg());
        color.setAlphaF(m_widget->focusFade() * 0.4 / 255.0);

        QPen pen(painter.pen());
        pen.setWidth(m_margin);
        pen.setColor(color);

        painter.setPen(pen);
        painter.drawRect(m_focusFadeBounds);
    }
}

bool
TermArrange::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        relayout(size());
        return true;
    }

    return QWidget::event(event);
}

void
TermArrange::resizeEvent(QResizeEvent *event)
{
    relayout(event->size());
}

QSize
TermArrange::sizeHint() const
{
    return m_sizeHint;
}

void
TermArrange::copyImage()
{
    QImage image(size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    render(&painter);
    QApplication::clipboard()->setImage(image);
}

bool
TermArrange::saveAsImage(QFile *file)
{
    QImage image(size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    render(&painter);
    image.save(file, "PNG");
    return file->error() == QFile::NoError;
}

//
// Separator widget
//
TermSeparator::TermSeparator(TermInstance *term, QWidget *parent) :
    QWidget(parent)
{
    connect(term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(rebackground(QRgb,QRgb)));
    m_color = term->fg();
    m_color.setAlphaF(0.4f);
}

void
TermSeparator::rebackground(QRgb, QRgb fg)
{
    m_color = fg;
    m_color.setAlphaF(0.4f);
    update();
}

void
TermSeparator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_color);
}
