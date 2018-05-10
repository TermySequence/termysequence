// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"
#include "lib/types.h"

#include <QWidget>
#include <QHash>

class TermScrollport;
class TermBuffers;
class TermInstance;

class TermModtimes final: public QWidget, public FontBase
{
    Q_OBJECT

private:
    TermScrollport *m_scrollport;
    TermBuffers *m_buffers;
    TermInstance *m_term;

    QHash<qreal,QString> m_info;

    index_t m_origin;
    int64_t m_startTime;
    int32_t m_originTime;

    bool m_visible;
    bool m_floating;
    bool m_haveStartTime;

    index_t m_wantsRow;
    int32_t m_wantsTime;
    QMetaObject::Connection m_mocWants;

    QSize m_sizeHint;

    bool tryWantsRow(index_t wantsRow, int32_t wantsTime);

private slots:
    void handleTimes();
    void setTimingOrigin(index_t row);
    void floatTimingOrigin();

    void refont(const QFont &font);

    void handleWantsRow();

    void pushTimeAttributes();
    void handleFollowing(bool following);
    void handleAttributeChanged(const QString &key, const QString &value);

protected:
    void paintEvent(QPaintEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    TermModtimes(TermScrollport *scrollport, QWidget *parent);

    index_t origin() const;
    int32_t originTime() const;

    void setWants(index_t wantsRow, int32_t wantsTime);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

public:
    static QString getTimeString64(int64_t value);
};
