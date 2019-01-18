// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "region.h"

#include <QObject>
#include <QPoint>
#include <QVector>

class TermBuffers;
class TermScrollport;

class Selection final: public QObject, public Region
{
    Q_OBJECT

private:
    TermBuffers *m_buffers;

    index_t anchorRow;
    column_t anchorCol;

    bool hasAnchor = false;

    QVector<std::pair<column_t,column_t>> getWords(index_t row) const;

    bool checkResult();
    void reportResult();
    void reportModified();

signals:
    void activated();
    void deactivated();
    void restarted();
    void modified();
    void activeHandleChanged(bool upper);

public:
    Selection(TermBuffers *buffer);

    inline bool isEmpty() const { return !hasAnchor || RegionBase::isEmpty(); }

    bool containsRel(size_t offset) const;
    bool containsRel(size_t offset, column_t col) const;
    bool isAfter(size_t offset) const;

    void setAnchor(const QPoint &p);
    bool setFloat(const QPoint &p);
    void finish(const QPoint &p);
    void clear();
    void moveAnchor(bool start);

    void selectBuffer();
    void selectView(const TermScrollport *scrollport);
    void selectRegion(const RegionBase *region);

    void selectWordAt(const QPoint &p);
    void selectLineAt(const QPoint &p);

    void selectWord(int index);
    void selectLine(int arg);
    void moveForwardWord();
    void moveBackWord();

    std::pair<index_t,bool> forwardChar(bool upper);
    std::pair<index_t,bool> backChar(bool upper);
    std::pair<index_t,bool> forwardWord(bool upper);
    std::pair<index_t,bool> backWord(bool upper);
    std::pair<index_t,bool> forwardLine(bool upper);
    std::pair<index_t,bool> backLine(bool upper);

    QString getLine() const;
};
