// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "viewport.h"

#include <QPoint>

class TermScreen final: public TermViewport
{
    Q_OBJECT

    friend class TermRowIterator;

private:
    QPoint m_cursor;
    QPoint m_origin;
    int m_row = 0;

signals:
    void cursorMoved(int row);

protected slots:
    void handleBufferChanged();
    void handleSizeChanged(QSize size);

public:
    TermScreen(TermInstance *term, const QSize &size, QObject *parent);

    inline QPoint cursor() const { return m_cursor; }
    inline QPoint origin() const { return m_origin; }
    inline int row() const { return m_row; }

    void setCursor(QPoint cursor);
};
