// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "bufferbase.h"
#include "region.h"
#include "regioniter.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTextStream>

BufferBase::BufferBase(TermInstance *term) :
    m_term(term),
    m_selection(nullptr)
{
}

BufferBase::BufferBase(TermInstance *term, Selection *selection) :
    m_term(term),
    m_selection(selection)
{
}

int
BufferBase::clipboardCopy() const
{
    RegionBase tmp(Tsqt::RegionInvalid);
    tmp.startRow = origin();
    tmp.endRow = tmp.startRow + size();
    tmp.startCol = 0;
    tmp.endCol = 0;

    RegionStringBuilder i(this, &tmp);
    QString result = i.build();
    if (!result.endsWith('\n'))
        result += '\n';

    QApplication::clipboard()->setText(result);
    return result.size();
}

bool
BufferBase::saveAs(QFile *file) const
{
    QTextStream out(file);

    for (size_t i = 0, n = size(); i < n; ++i) {
        const auto &crow = row(i);
        out << QByteArray(crow.str.data(), crow.str.size()) << '\n';
    }

    return file->error() == QFile::NoError;
}
