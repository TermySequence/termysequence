// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "bufferbase.h"

#include <QObject>
#include <QHash>
#include <deque>

class TermManager;

class TermOverlay final: public QObject, public BufferBase
{
    Q_OBJECT

private:
    std::deque<CellRow> m_rows;
    QHash<QChar,int> m_menu;

    CellRow *m_serverLine, *m_peerLine;

    CellRow &addLine(const QString &text);
    void addBoldLine(const QString &text);
    void addDoubleLine(const QString &text);
    void addBlankLine();
    void setLine(CellRow *row, const QString &text);

private slots:
    void handlePeerChanging();
    void handlePeerChanged();

    void handleServerFullname(QString fullname);
    void handlePeerFullname(QString fullname);

public:
    TermOverlay(TermInstance *term);

    inline size_t offset() const { return 0; }
    inline size_t size() const { return m_rows.size(); }
    inline index_t origin() const { return 0; }
    inline uint8_t caporder() const { return 8; }
    inline bool noScrollback() const { return true; }

    inline const CellRow& row(size_t i) const { return m_rows.at(i); }

public:
    void inputEvent(TermManager *, QByteArray&);
};
