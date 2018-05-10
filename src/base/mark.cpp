// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/color.h"
#include "app/config.h"
#include "mark.h"
#include "term.h"
#include "server.h"
#include "scrollport.h"
#include "region.h"
#include "manager.h"
#include "mainwindow.h"

#include <QMenu>
#include <QPainter>
#include <QContextMenuEvent>

#define TR_TEXT1 TL("window-text", "Note") + A(": ")
#define TR_TEXT2 TL("window-text", "Author") + A(": ")
#define TR_TEXT3 TL("window-text", "Author unknown")
#define TR_TEXT4 TL("window-text", "Location of current search match")
#define TR_TEXT5 TL("window-text", "Command") + A(": ")
#define TR_TEXT6 TL("window-text", "Exit status %1")
#define TR_TEXT7 TL("window-text", "Killed by signal %1")
#define TR_TEXT8 TL("window-text", "Row") + A(": ")
#define TR_TEXT9 TL("window-text", "Running")

static const char *const s_base36 = "0123456789abcdefghijklmnopqrstuvwxyz";

TermMark::TermMark(TermScrollport *scrollport, QWidget *parent) :
    QWidget(parent),
    m_regionId(INVALID_REGION_ID),
    m_term(scrollport->term()),
    m_scrollport(scrollport)
{
}

void
TermMark::getSearchMark(const Region *region, QString &text, int &bgi, int &fgi)
{
    bgi = PALETTE_SH_MARK_MATCH_BG;
    fgi = PALETTE_SH_MARK_MATCH_FG;
    text = g_str_MARK_M + '>';
}

QString
TermMark::getNoteText(const QString &notechar)
{
    return g_str_MARK_N + (notechar.isEmpty() ? C('>') : notechar.at(0));
}

void
TermMark::getNoteColors(int &bgi, int &fgi)
{
    bgi = PALETTE_SH_MARK_NOTE_BG;
    fgi = PALETTE_SH_MARK_NOTE_FG;
}

void
TermMark::getNoteMark(const Region *region, QString &text, int &bgi, int &fgi)
{
    QChar c2('>');

    QString notechar = region->attributes.value(g_attr_REGION_NOTECHAR);
    if (!notechar.isEmpty())
        c2 = notechar.at(0);

    bgi = PALETTE_SH_MARK_NOTE_BG;
    fgi = PALETTE_SH_MARK_NOTE_FG;

    text = g_str_MARK_N;
    text.append(c2);
}

QString
TermMark::getJobText(const QString &codestr)
{
    QString c1;
    QChar c2;

    // Note: more text logic in getJobMark

    if (!codestr.isEmpty()) {
        int code = codestr.toInt();

        if (code <= 128) {
            c1 = g_str_MARK_E;
        } else {
            c1 = g_str_MARK_S;
            code -= 128;
        }

        if (code < 0)
            c2 = '?';
        else if (code >= 0 && code < 36)
            c2 = s_base36[code];
        else if (code == 127)
            c2 = '-';
        else
            c2 = '+';
    } else {
        c1 = g_str_MARK_R;
        c2 = '<';
    }

    return c1 + c2;
}

void
TermMark::getJobColors(const QString &codestr, int &bgi, int &fgi)
{
    // Note: more colors logic in getJobMark

    if (!codestr.isEmpty()) {
        int code = codestr.toInt();

        if (code == 0) {
            bgi = PALETTE_SH_MARK_EXIT0_BG;
            fgi = PALETTE_SH_MARK_EXIT0_FG;
        } else if (code <= 128) {
            bgi = PALETTE_SH_MARK_EXITN_BG;
            fgi = PALETTE_SH_MARK_EXITN_FG;
        } else {
            bgi = PALETTE_SH_MARK_SIGNAL_BG;
            fgi = PALETTE_SH_MARK_SIGNAL_FG;
        }
    } else {
        bgi = PALETTE_SH_MARK_RUNNING_BG;
        fgi = PALETTE_SH_MARK_RUNNING_FG;
    }
}

QString
TermMark::getJobMark(const Region *region, QString &tooltip, int &bgi, int &fgi)
{
    QString c1;
    QChar c2;

    if (!region)
        return c1;

    QString cmd = region->attributes.value(g_attr_REGION_COMMAND);
    if (cmd.isEmpty()) {
        ; // do nothing
    }
    else if (cmd.size() < 24) {
        tooltip += '\n' + TR_TEXT5 + cmd;
    }
    else {
        tooltip += '\n' + TR_TEXT5 + cmd.left(24) + A("...");
    }

    auto i = region->attributes.constFind(g_attr_REGION_EXITCODE);
    if (i != region->attributes.cend()) {
        int code = i->toInt();

        if (code < 0 || region->flags & Tsq::EmptyCommand) {
            return c1;
        } else if (code == 0) {
            c1 = g_str_MARK_E;
            bgi = PALETTE_SH_MARK_EXIT0_BG;
            fgi = PALETTE_SH_MARK_EXIT0_FG;
            tooltip += '\n' + TR_TEXT6.arg(code);
        } else if (code <= 128) {
            c1 = g_str_MARK_E;
            bgi = PALETTE_SH_MARK_EXITN_BG;
            fgi = PALETTE_SH_MARK_EXITN_FG;
            tooltip += '\n' + TR_TEXT6.arg(code);
        } else {
            c1 = g_str_MARK_S;
            code -= 128;
            bgi = PALETTE_SH_MARK_SIGNAL_BG;
            fgi = PALETTE_SH_MARK_SIGNAL_FG;
            tooltip += '\n' + TR_TEXT7.arg(code);
        }

        if (code < 0)
            c2 = '?';
        else if (code >= 0 && code < 36)
            c2 = s_base36[code];
        else if (code == 127)
            c2 = '-';
        else
            c2 = '+';
    } else if (region->flags & Tsq::HasCommand) {
        c1 = g_str_MARK_R;
        c2 = '<';
        bgi = PALETTE_SH_MARK_RUNNING_BG;
        fgi = PALETTE_SH_MARK_RUNNING_FG;
        tooltip += '\n' + TR_TEXT9;
    } else if (region->flags & Tsq::HasPrompt) {
        c1 = g_str_MARK_P;
        c2 = '>';
        bgi = PALETTE_SH_MARK_PROMPT_BG;
        fgi = PALETTE_SH_MARK_PROMPT_FG;
    } else {
        return c1;
    }

    return c1 + c2;
}

QString
TermMark::getNoteTooltip(const Region *region, const TermInstance *term)
{
    QString tooltip = TR_TEXT1 + region->attributes.value(g_attr_REGION_NOTETEXT);

    if (tooltip.size() >= SEMANTIC_TOOLTIP_MAX)
        tooltip = tooltip.left(SEMANTIC_TOOLTIP_MAX) + C(0x2026);

    QString author = L("%1@%2").arg(region->attributes.value(g_attr_REGION_USER),
                                    region->attributes.value(g_attr_REGION_HOST));

    if (author.isEmpty())
        tooltip.prepend(TR_TEXT3 + '\n');
    else if (author != term->server()->shortname())
        tooltip.prepend(TR_TEXT2 + author + '\n');

    return tooltip;
}

bool
TermMark::setRegion(const Region *region)
{
    int bgi, fgi;
    QString tooltip;

    m_regionId = region->id();
    m_regionType = region->type();

    switch (m_regionType) {
    case Tsqt::RegionSearch:
        getSearchMark(region, m_text, bgi, fgi);
        tooltip = TR_TEXT4;
        break;
    case Tsqt::RegionUser:
        getNoteMark(region, m_text, bgi, fgi);
        tooltip = getNoteTooltip(region, m_term);
        break;
    case Tsqt::RegionJob:
        m_text = getJobMark(region, tooltip, bgi, fgi);
        if (!m_text.isEmpty())
            break;
        // fallthru
    default:
        return false;
    }

    setToolTip(TR_TEXT8 + QString::number(region->startRow) + '\n' + tooltip);

    const TermPalette &palette = m_term->palette();
    QRgb bg = palette.at(bgi), fg = palette.at(fgi);

    // Set color state
    if (m_regionType == Tsqt::RegionJob && m_regionId == m_scrollport->activeJobId()) {
        QRgb abg = palette.at(PALETTE_SH_MARK_SELECTED_BG);
        if (PALETTE_IS_ENABLED(abg))
            bg = abg;
        QRgb afg = palette.at(PALETTE_SH_MARK_SELECTED_FG);
        if (PALETTE_IS_ENABLED(afg))
            fg = afg;
    }

    m_bg = (m_haveBg = PALETTE_IS_ENABLED(bg)) ? bg : m_term->bg();
    m_fg = PALETTE_IS_ENABLED(fg) ? fg : m_term->fg();
    m_blend = Colors::blend1(m_bg, m_fg);

    update();
    return true;
}

void
TermMark::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (m_hover)
        painter.fillRect(rect(), m_blend);
    else if (m_haveBg)
        painter.fillRect(rect(), m_bg);

    QFont font = m_term->font();
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(m_fg);
    painter.drawText(rect(), m_text);
}

void
TermMark::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = mapToParent(event->pos());
    QMenu *m;

    m_scrollport->setClickPoint(height(), parentWidget()->height(), pos.y());

    switch (m_regionType) {
    case Tsqt::RegionJob:
        m = m_scrollport->manager()->parent()->getJobPopup(m_term, m_regionId, this);
        break;
    case Tsqt::RegionUser:
        m = m_scrollport->manager()->parent()->getNotePopup(m_term, m_regionId, this);
        break;
    default:
        event->ignore();
        return;
    }

    m->popup(event->globalPos());
    event->accept();
}

void
TermMark::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_regionType == Tsqt::RegionJob)
            m_scrollport->setActiveJob(m_regionId);

        event->accept();
    }
}

void
TermMark::enterEvent(QEvent *event)
{
    m_hover = true;
    update();
}

void
TermMark::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
}

QChar
TermMark::base36(int num, char fallback)
{
    return (num < 36) ? s_base36[num] : fallback;
}
