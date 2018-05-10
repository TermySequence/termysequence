// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/wire.h"

#include <QString>
#include <QVector>
#include <QHash>

#define SPLIT_EMPTY     0
#define SPLIT_LOCAL     1
#define SPLIT_REMOTE    2
#define SPLIT_HRESIZE2  3
#define SPLIT_HRESIZE3  4
#define SPLIT_HRESIZE4  5
#define SPLIT_VRESIZE2  6
#define SPLIT_VRESIZE3  7
#define SPLIT_VRESIZE4  8
#define SPLIT_HFIXED2   9
#define SPLIT_HFIXED3   10
#define SPLIT_HFIXED4   11
#define SPLIT_VFIXED2   12
#define SPLIT_VFIXED3   13
#define SPLIT_VFIXED4   14

#define MAX_SPLIT_WIDGETS 4

struct TermState
{
    Tsq::Uuid termId;
    Tsq::Uuid serverId;
    QString profileName;
    bool islocal;
};

struct ScrollportState
{
    Tsq::Uuid id;
    uint64_t offset;
    uint64_t modtimeRow;
    int32_t modtime;
    uint32_t activeJob;
};

//
// Parser
//
class SplitLayoutReader
{
private:
    QByteArray m_bytes;
    Tsq::ProtocolUnmarshaler m_unm;
    bool m_ok;
    unsigned m_focusPos;

    QVector<unsigned> m_types;
    QVector<TermState> m_terms;
    QVector<unsigned> m_viewportCounts;
    QVector<ScrollportState> m_viewports;
    QVector<QList<int>> m_sizes;

    bool parseItem(unsigned depth);
    void parseStateRecords();

public:
    SplitLayoutReader();
    void parse(const QByteArray &bytes);
    unsigned nextType();

    // Pane parameters
    TermState nextTermRecord();
    unsigned nextViewportCount();
    ScrollportState nextViewportRecord();

    // Split parameters
    QList<int> nextSizes();

    inline unsigned focusPos() const { return m_ok ? m_focusPos : 0; }
};

//
// Serializer
//
class SplitLayoutWriter
{
private:
    Tsq::ProtocolMarshaler m_marshaler;

public:
    SplitLayoutWriter();
    void setFocusPos(unsigned focusPos);

    void addEmptyPane();
    void addPane(const TermState &term, const QVector<ScrollportState> &state);

    void addResize(bool horizontal, const QList<int> &sizes);
    void addFixed(bool horizontal, const QList<int> &sizes);

    QByteArray result() const;
};
