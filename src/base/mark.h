// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/enums.h"
#include "lib/types.h"

#include <QWidget>

class TermInstance;
class TermScrollport;
class Region;

class TermMark final: public QWidget
{
private:
    regionid_t m_regionId;
    Tsqt::RegionType m_regionType;

    TermInstance *m_term;
    TermScrollport *m_scrollport;

    QString m_text;
    QColor m_fg, m_bg, m_blend;

    bool m_haveBg = false;
    bool m_hover = false;

protected:
    void paintEvent(QPaintEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

public:
    TermMark(TermScrollport *scrollport, QWidget *parent);

    inline regionid_t regionId() const { return m_regionId; }

    bool setRegion(const Region *region);

public:
    // Mark string/color calculation
    static void getSearchMark(const Region *region, QString &text, int &bgi, int &fgi);

    static void getNoteMark(const Region *region, QString &text, int &bgi, int &fgi);
    static QString getNoteText(const QString &notechar);
    static void getNoteColors(int &bgi, int &fgi);
    static QString getNoteTooltip(const Region *region, const TermInstance *term);

    static QString getJobMark(const Region *region, QString &tooltip, int &bgi, int &fgi);
    static QString getJobText(const QString &codestr);
    static void getJobColors(const QString &codestr, int &bgi, int &fgi);

    // Base36 characters
    static QChar base36(int num, char fallback);
};
