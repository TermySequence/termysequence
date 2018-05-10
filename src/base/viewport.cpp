// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "viewport.h"
#include "term.h"
#include "buffers.h"
#include "regioniter.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>

TermViewport::TermViewport(TermInstance *term, const QSize &size, QObject *parent):
    QObject(parent),
    BlinkBase{},
    m_term(term),
    m_buffers(term->buffers()),
    m_bounds(size)
{
    connect(this, SIGNAL(offsetChanged(int)), SLOT(updateRegions()));
    connect(m_buffers, SIGNAL(regionChanged()), SLOT(updateRegions()));
}

void
TermViewport::moveToEnd()
{
    int offset = m_buffers->size() - m_bounds.height();

    m_offset = (offset > 0) ? offset : 0;
    emit offsetChanged(m_offset);
}

void
TermViewport::moveToOffset(int pos)
{
    int offset = m_buffers->size() - m_bounds.height();

    if (offset < 0)
        offset = 0;
    if (offset > pos)
        offset = pos;

    emit offsetChanged(m_offset = offset);
}

void
TermViewport::moveToRow(index_t *row, bool exact)
{
    index_t origin = m_buffers->origin();

    if (exact) {
        int offset = m_buffers->size() - m_bounds.height();

        if (offset < 0)
            *row = 0l;
        else if (*row < origin)
            *row = origin;
        else if (*row > origin + offset)
            *row = origin + offset;
    } else {
        int offset = m_buffers->size() - 1;

        if (offset < 0)
            *row = 0l;
        else if (*row < origin)
            *row = origin;
        else if (*row > origin + offset)
            *row = origin + offset;

        index_t start = origin + m_offset;
        index_t end = start + m_bounds.height();

        if (*row >= end)
            *row -= m_bounds.height() - 1;
        else if (*row >= start)
            *row = start;
    }

    emit offsetChanged(m_offset = *row - origin);
}

void
TermViewport::updateRegions()
{
    m_buffers->updateRows(m_offset, m_offset + m_bounds.height(), nullptr);
}

void
TermViewport::setFocusEffect(int focusEffect)
{
    m_focusEffect = focusEffect;
    emit focusEffectChanged(focusEffect);
}

void
TermViewport::setResizeEffect(int resizeEffect)
{
    m_resizeEffect = resizeEffect;
    emit resizeEffectChanged(resizeEffect);
}

QPoint
TermViewport::mousePos() const
{
    return QPoint(-1, -1);
}

QString
TermViewport::toString() const
{
    RegionBase tmp(Tsqt::RegionInvalid);
    tmp.startRow = m_buffers->origin() + m_offset;
    tmp.endRow = tmp.startRow + height();
    tmp.startCol = 0;
    tmp.endCol = 0;

    RegionStringBuilder i(m_buffers, &tmp);
    QString result = i.build();
    if (!result.endsWith('\n'))
        result += '\n';

    return result;
}

int
TermViewport::clipboardCopy() const
{
    QString text = toString();
    QApplication::clipboard()->setText(text);
    return text.size();
}

bool
TermViewport::saveAs(QFile *file) const
{
    QTextStream(file) << toString();
    return file->error() == QFile::NoError;
}
