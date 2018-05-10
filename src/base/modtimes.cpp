// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "modtimes.h"
#include "scrollport.h"
#include "term.h"
#include "server.h"
#include "buffers.h"
#include "region.h"
#include "manager.h"
#include "listener.h"
#include "mainwindow.h"

#include <QMenu>
#include <QPainter>
#include <QContextMenuEvent>
#include <QtMath>

TermModtimes::TermModtimes(TermScrollport *scrollport, QWidget *parent) :
    QWidget(parent),
    m_scrollport(scrollport),
    m_buffers(scrollport->buffers()),
    m_term(scrollport->term()),
    m_origin(INVALID_INDEX),
    m_originTime(INVALID_MODTIME),
    m_visible(false),
    m_floating(true)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Get start time
    const auto &attr = m_term->server()->attributes();
    m_startTime = attr.value(g_attr_STARTED).toLongLong(&m_haveStartTime, 10) / 100;

    connect(m_buffers, SIGNAL(contentChanged()), SLOT(handleTimes()));
    connect(m_scrollport, SIGNAL(offsetChanged(int)), SLOT(handleTimes()));
    connect(m_scrollport, SIGNAL(setTimingOriginRequest(index_t)), SLOT(setTimingOrigin(index_t)));
    connect(m_scrollport, SIGNAL(floatTimingOriginRequest()), SLOT(floatTimingOrigin()));
    connect(m_scrollport, SIGNAL(primarySet()), SLOT(pushTimeAttributes()));
    connect(m_scrollport, SIGNAL(followingChanged(bool)), SLOT(handleFollowing(bool)));

    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(update()));
    connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_term, SIGNAL(ownershipChanged(bool)), SLOT(pushTimeAttributes()));
    connect(m_term, SIGNAL(attributeChanged(QString,QString)), SLOT(handleAttributeChanged(const QString&,const QString&)));

    refont(m_term->font());
}

index_t
TermModtimes::origin() const
{
    return !m_floating ? m_origin : INVALID_INDEX;
}

int32_t
TermModtimes::originTime() const
{
    return !m_floating ? m_originTime : INVALID_MODTIME;
}

void
TermModtimes::pushTimeAttributes()
{
    if (m_scrollport->primary() && m_term->ours()) {
        QString spec;
        if (!m_floating) {
            spec = L("%1,%2").arg(m_origin).arg(m_originTime);
        }
        if (m_term->attributes().value(g_attr_PREF_MODTIME) != spec)
            g_listener->pushTermAttribute(m_term, g_attr_PREF_MODTIME, spec);
    }
}

void
TermModtimes::handleAttributeChanged(const QString &key, const QString &value)
{
    if (m_scrollport->following() && key == g_attr_PREF_MODTIME) {
        QStringList spec = value.split(',');
        index_t row;
        int32_t modtime;
        bool ok1 = false, ok2 = false;

        if (spec.size() == 2) {
            row = spec[0].toULongLong(&ok1);
            modtime = spec[1].toInt(&ok2);
        }
        if (!ok1 || !ok2 || !tryWantsRow(row, modtime)) {
            m_floating = true;
            handleTimes();
        }
    }
}

void
TermModtimes::handleFollowing(bool following)
{
    if (following) {
        const auto &a = m_term->attributes();
        handleAttributeChanged(g_attr_PREF_MODTIME, a.value(g_attr_PREF_MODTIME));
    }
}

QString
TermModtimes::getTimeString64(int64_t value)
{
    QString rc;

    if (value < 0l)
        rc = C('-');

    value = std::labs(value);

    if (value < 60000l) {
        return rc + QString::number(value / 1000.0, 'f', 2) + 's';
    }
    if (value < 3600000l) {
        int m = value / 60000l;
        int s = (value / 1000l) % 60;
        return rc.append(A("%1m%2s")).arg(m).arg(s);
    }
    if (value < 86400000l) {
        int h = value / 3600000l;
        int m = (value % 3600000l) / 60000l;
        return rc.append(A("%1h%2m")).arg(h).arg(m);
    }
    if (value < 8640000000l) {
        int d = value / 86400000l;
        int h = (value % 86400000l) / 3600000l;
        return rc.append(A("%1d%2h")).arg(d).arg(h);
    }
    return rc.append(A(">100d"));
}

static inline QString
timeString(int value)
{
    QString rc(value > 0 ? '+' : '-');
    value = std::labs(value);

    if (value < 600) {
        return rc + QString::number(value / 10.0, 'f', 1) + 's';
    }
    if (value < 36000) {
        int m = value / 600;
        int s = (value / 10) % 60;
        return rc.append(A("%1m%2s")).arg(m).arg(s);
    }
    if (value < 864000) {
        int h = value / 36000;
        int m = (value % 36000) / 600;
        return rc.append(A("%1h%2m")).arg(h).arg(m);
    }
    if (value < 86400000) {
        int d = value / 864000;
        int h = (value % 864000) / 36000;
        return rc.append(A("%1d%2h")).arg(d).arg(h);
    }
    return rc.append(A(">100d"));
}

void
TermModtimes::handleTimes()
{
    index_t min = m_buffers->origin();
    index_t max = min + m_buffers->size();
    index_t start = min + m_scrollport->offset();
    index_t end = start + m_scrollport->height();

    if (end > max)
        end = max;
    if (start >= end || m_term->overlayActive())
        return;

    index_t origin = m_origin;
    int32_t originTime = m_originTime;

    if (m_floating || origin >= max || originTime == INVALID_MODTIME) {
        const Region *r = m_buffers->modtimeRegion();
        if (r) {
            origin = m_origin = r->startRow;
            originTime = m_originTime = m_buffers->row(origin - min).modtime;
        }
        if (m_originTime == INVALID_MODTIME) {
            for (origin = end - 1;; --origin) {
                originTime = m_buffers->row(origin - min).modtime;

                if (originTime != INVALID_MODTIME)
                    break;
                if (origin == start)
                    return;
            }
        }
    }

    if (!m_visible)
        return;

    int32_t last = INVALID_MODTIME, y = 0;
    QString lastStr;
    m_info.clear();
    update();

    while (start < end) {
        if (start == origin) {
            m_info.insert(y * m_cellSize.height(), L("\u25cf 0.0s"));
        }
        else {
            const CellRow &row = m_buffers->row(start - min);
            int32_t modtime = row.modtime;
            int32_t cur = modtime - originTime;
            if (cur && last != cur && modtime != INVALID_MODTIME &&
                !(row.flags & Tsqt::NoSelect))
            {
                QString str = timeString(last = cur);
                if (lastStr != str) {
                    m_info.insert(y * m_cellSize.height(), lastStr = str);
                }
            }
        }
        ++y;
        ++start;
    }
}

void
TermModtimes::setTimingOrigin(index_t row)
{
    index_t min = m_buffers->origin();
    index_t max = min + m_buffers->size();

    if (row < min)
        row = min;
    if (row >= max)
        return;

    m_origin = row;
    m_originTime = m_buffers->row(row - min).modtime;
    m_floating = false;

    handleTimes();
    pushTimeAttributes();
}

void
TermModtimes::floatTimingOrigin()
{
    m_floating = true;
    handleTimes();
    pushTimeAttributes();
}

void
TermModtimes::refont(const QFont &font)
{
    calculateCellSize(font);

    int w = qCeil(7 * m_cellSize.width());

    m_sizeHint = QSize(w, w);

    handleTimes();
    updateGeometry();
}

void
TermModtimes::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setPen(m_term->fg());
    painter.setFont(m_term->font());

    QRectF rect(0.0, 0.0, width(), m_cellSize.height());

    for (auto i = m_info.begin(), j = m_info.end(); i != j; ++i) {
        rect.moveTop(i.key());
        painter.drawText(rect, i.value(), QTextOption(Qt::AlignRight));
    }
}

void
TermModtimes::contextMenuEvent(QContextMenuEvent *event)
{
    m_scrollport->setClickPoint(m_cellSize.height(), height(), event->y());

    QMenu *m = m_scrollport->manager()->parent()->getModtimePopup(this);
    m->popup(event->globalPos());
    event->accept();
}

void
TermModtimes::showEvent(QShowEvent *event)
{
    m_visible = true;
    handleTimes();
}

void
TermModtimes::hideEvent(QHideEvent *event)
{
    m_visible = false;
}

QSize
TermModtimes::sizeHint() const
{
    return m_sizeHint;
}

QSize
TermModtimes::minimumSizeHint() const
{
    return QSize();
}

bool
TermModtimes::tryWantsRow(index_t wantsRow, int32_t wantsTime)
{
    index_t origin = m_buffers->origin();
    size_t size = m_buffers->size();

    // Determine if the specified row is in bounds
    if (wantsRow < origin + size) {
        m_origin = wantsRow;
        m_originTime = wantsTime;
        m_floating = false;

        handleTimes();
        pushTimeAttributes();
        return true;
    }

    return false;
}

void
TermModtimes::handleWantsRow()
{
    disconnect(m_mocWants);
    tryWantsRow(m_wantsRow, m_wantsTime);
}

void
TermModtimes::setWants(index_t wantsRow, int32_t wantsTime)
{
    bool populating = m_scrollport->manager()->populating();

    if (wantsRow != INVALID_INDEX && !tryWantsRow(wantsRow, wantsTime) && populating)
    {
        m_wantsRow = wantsRow;
        m_wantsTime = wantsTime;
        m_mocWants = connect(m_buffers, SIGNAL(bufferChanged()), SLOT(handleWantsRow()));
    }
}
