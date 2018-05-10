// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "marks.h"
#include "mark.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "region.h"
#include "manager.h"
#include "mainwindow.h"

#include <QMenu>
#include <QtMath>
#include <QContextMenuEvent>

TermMarks::TermMarks(TermScrollport *scrollport, QWidget *parent) :
    QWidget(parent),
    m_scrollport(scrollport),
    m_term(scrollport->term())
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    connect(m_term->buffers(), SIGNAL(regionChanged()), SLOT(handleRegions()));
    connect(m_scrollport, SIGNAL(offsetChanged(int)), SLOT(handleRegions()));
    connect(m_scrollport, SIGNAL(searchUpdate()), SLOT(handleRegions()));
    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(handleRegions()));
    connect(m_term, SIGNAL(paletteChanged()), SLOT(handleRegions()));
    connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));

    refont(m_term->font());
}

void
TermMarks::handleRegions()
{
    if (!m_visible)
        return;

    QHash<int,TermMark*> positions;
    int index = 0;

    for (auto &i: m_scrollport->regions().list) {
        const Region *r = i.cref();
        TermMark *mark;

        if (index == m_marks.size()) {
            mark = new TermMark(m_scrollport, this);
            mark->resize(m_markSize);
            m_marks.append(mark);
        } else {
            mark = m_marks.at(index);
        }

        if (mark->setRegion(r)) {
            int y = r->startRow - m_scrollport->offset() - r->buffer()->origin();

            auto i = positions.find(y);
            if (i != positions.cend()) {
                (*i)->setVisible(false);
                *i = mark;
            } else {
                positions.insert(y, mark);
            }

            mark->move(0, y * m_cellSize.height());
            mark->show();
            ++index;
        }
    }

    if (m_hiddenThreshold != index) {
        int tmp = m_hiddenThreshold;
        m_hiddenThreshold = index;

        for (; index < tmp; ++index) {
            m_marks.at(index)->setVisible(false);
        }
    }
}

void
TermMarks::refont(const QFont &font)
{
    calculateCellSize(font);

    int w = qCeil(2 * m_cellSize.width());

    m_markSize = QSize(w, qCeil(m_cellSize.height()));
    m_sizeHint = QSize(w, w);

    for (auto mark: qAsConst(m_marks))
        mark->resize(m_markSize);

    handleRegions();
    updateGeometry();
}

void
TermMarks::contextMenuEvent(QContextMenuEvent *event)
{
    m_scrollport->setClickPoint(m_cellSize.height(), height(), event->y());

    QMenu *m = m_scrollport->manager()->parent()->getMarksPopup(this);
    m->popup(event->globalPos());
    event->accept();
}

void
TermMarks::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_scrollport->setClickPoint(m_cellSize.height(), height(), event->y());
        m_scrollport->selectRegionByClickPoint(Tsqt::RegionJob, false);
        event->accept();
    }
}

void
TermMarks::showEvent(QShowEvent *event)
{
    m_visible = true;
    handleRegions();
}

void
TermMarks::hideEvent(QHideEvent *event)
{
    m_visible = false;
}

QSize
TermMarks::sizeHint() const
{
    return m_sizeHint;
}

QSize
TermMarks::minimumSizeHint() const
{
    return QSize();
}
