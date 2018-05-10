// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"
#include "lib/types.h"

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE
class TermInstance;
class Selection;

class BufferBase
{
protected:
    TermInstance *m_term;
    Selection *m_selection;

public:
    BufferBase(TermInstance *term);
    BufferBase(TermInstance *term, Selection *selection);
    virtual ~BufferBase() = default;

    inline TermInstance* term() { return m_term; }
    inline const TermInstance* term() const { return m_term; }
    inline Selection* selection() { return m_selection; }
    inline void setSelection(Selection *selection) { m_selection = selection; }

    virtual size_t size() const = 0;
    virtual size_t offset() const = 0;
    virtual index_t origin() const = 0;
    virtual uint8_t caporder() const = 0;
    virtual bool noScrollback() const = 0;

    virtual const CellRow& row(size_t i) const = 0;

    int clipboardCopy() const;
    bool saveAs(QFile *file) const;
};
