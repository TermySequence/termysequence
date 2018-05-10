// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "overlay.h"
#include "term.h"
#include "buffers.h"
#include "server.h"
#include "manager.h"
#include "lib/grapheme.h"

#define TR_FIELD1 TL("input-field", "From")
#define TR_FIELD2 TL("input-field", "To")
#define TR_OVERLAY1 TL("overlay-menu", "d - Disconnect")
#define TR_OVERLAY2 TL("overlay-menu", "c - Close this overlay and show the terminal")
#define TR_OVERLAY3 TL("overlay-menu", "h - Hide this terminal")
#define TR_OVERLAY4 TL("overlay-menu", "h - Hide this connection")
#define TR_OVERLAY5 TL("overlay-menu", "l - New Local Terminal (Profile: %1)")
#define TR_OVERLAY6 TL("overlay-menu", "r - New Remote Terminal (Profile: %1)")
#define TR_TEXT1 TL("window-text", "This terminal is hosting a connection between %1 servers")
#define TR_TEXT2 TL("window-text", "This is a direct connection between %1 servers")
#define TR_TEXT3 TL("window-text", "Connection Established")
#define TR_TEXT4 TL("window-text", \
    "Switch to another terminal to continue working or choose a menu option") + ':'

#define MENU_DISCONNECT 0
#define MENU_CLOSE      1
#define MENU_HIDE       2
#define MENU_LOCAL      3
#define MENU_REMOTE     4

TermOverlay::TermOverlay(TermInstance *term) :
    QObject(term),
    BufferBase(term)
{
    QString str = term->isTerm() ? TR_TEXT1 : TR_TEXT2;

    addBlankLine();
    addDoubleLine(QString("\xF0\x9F\x93\xA1") + TR_TEXT3);
    addBlankLine();
    addLine(str.arg(QString("\xF0\x9F\x96\xA5\xEF\xB8\x8F" FRIENDLY_NAME)));
    addBlankLine();
    addBlankLine();
    addBlankLine();
    addBlankLine();
    addLine(TR_TEXT4);
    addBlankLine();

    str = TR_OVERLAY1;
    m_menu[str.at(0)] = MENU_DISCONNECT;
    addBoldLine(str);

    if (term->isTerm()) {
        str = TR_OVERLAY2;
        m_menu[str.at(0)] = MENU_CLOSE;
        addBoldLine(str);

        str = TR_OVERLAY3;
    } else {
        str = TR_OVERLAY4;
    }

    m_menu[str.at(0)] = MENU_HIDE;
    addBoldLine(str);

    str = TR_OVERLAY5.arg(term->profileName());
    m_menu[str.at(0)] = MENU_LOCAL;
    addBoldLine(str);

    str = TR_OVERLAY6.arg(g_str_DEFAULT_PROFILE);
    m_menu[str.at(0)] = MENU_REMOTE;
    addBoldLine(str);

    m_serverLine = &m_rows[6];
    m_peerLine = &m_rows[7];
    ServerInstance *server = term->server();

    connect(server, SIGNAL(fullnameChanged(QString)), SLOT(handleServerFullname(QString)));
    handleServerFullname(server->fullname());

    connect(term, SIGNAL(peerChanging()), SLOT(handlePeerChanging()));
    connect(term, SIGNAL(peerChanged()), SLOT(handlePeerChanged()));
    handlePeerChanged();
}

void
TermOverlay::handlePeerChanging()
{
    m_term->peer()->disconnect(this);
}

void
TermOverlay::handlePeerChanged()
{
    auto *peer = m_term->peer();

    if (peer) {
        connect(peer, SIGNAL(fullnameChanged(QString)), SLOT(handlePeerFullname(QString)));
        handlePeerFullname(peer->fullname());
    } else {
        handlePeerFullname(g_mtstr);
    }
}

void
TermOverlay::handleServerFullname(QString fullname)
{
    setLine(m_serverLine, L("%1: %2").arg(TR_FIELD1, fullname));
    m_term->buffers()->reportOverlayChanged();
}

void
TermOverlay::handlePeerFullname(QString fullname)
{
    setLine(m_peerLine, L("%1: %2").arg(TR_FIELD2, fullname));
    m_term->buffers()->reportOverlayChanged();
}

void
TermOverlay::inputEvent(TermManager *manager, QByteArray &result)
{
    QString tmp(result);

    if (!tmp.isEmpty()) {
        QChar c = tmp.at(0);
        // Allow control characters through
        if (c.unicode() < 0x20)
            return;

        result.clear();

        auto i = m_menu.constFind(c);
        if (i != m_menu.cend())
            switch (*i) {
            case MENU_DISCONNECT:
                manager->invokeSlot(A("DisconnectTerminal"), true);
                break;
            case MENU_CLOSE:
                m_term->hideOverlay();
                break;
            case MENU_HIDE:
                manager->invokeSlot(A("HideTerminalEverywhere"), true);
                break;
            case MENU_LOCAL:
                manager->invokeSlot(A("CloneTerminal"), true);
                break;
            case MENU_REMOTE:
                if (m_term->peer()) {
                    tmp = A("NewTerminal||");
                    tmp.append(m_term->peer()->idStr());
                    manager->invokeSlot(tmp, true);
                }
                break;
            }
    }
}

void
TermOverlay::setLine(CellRow *row, const QString &text)
{
    row->cells.clear();
    row->str = text.toStdString();
    row->size = 0;
    unsigned x = 0;

    Tsq::CategoryWalk cbf(m_term->unicoding(), row->str);
    Tsq::CellFlags flags;
    column_t size;

    while ((size = cbf.next(flags)))
    {
        Cell cell;
        cell.flags = flags;
        cell.startptr = cbf.start();
        cell.endptr = cbf.end();
        cell.cellx = x;
        cell.cellwidth = size * (1 + !!(flags & Tsq::DblWidthChar));
        row->cells.emplace_back(cell);

        x += cell.cellwidth;
        row->size += size;
    }
}

void
TermOverlay::addBlankLine()
{
    m_rows.emplace_back();
    m_rows.back().flags = Tsqt::Downloaded|Tsqt::NoSelect;
}

CellRow &
TermOverlay::addLine(const QString &text)
{
    m_rows.emplace_back();
    CellRow &row = m_rows.back();
    row.flags = Tsqt::Downloaded|Tsqt::NoSelect;
    setLine(&row, text);
    return row;
}

void
TermOverlay::addBoldLine(const QString &text)
{
    CellRow &row = addLine(text.left(1));
    Cell &boldCell = row.cells.front();
    boldCell.flags |= Tsq::Bold;
    unsigned x = boldCell.cellwidth;
    size_t offset = boldCell.endptr;

    std::string rest = text.mid(1).toStdString();
    row.str += rest;

    Tsq::CategoryWalk cbf(m_term->unicoding(), rest);
    Tsq::CellFlags flags;
    column_t size;

    while ((size = cbf.next(flags)))
    {
        Cell cell;
        cell.flags = flags;
        cell.startptr = offset + cbf.start();
        cell.endptr = offset + cbf.end();
        cell.cellx = x;
        cell.cellwidth = size * (1 + !!(flags & Tsq::DblWidthChar));
        row.cells.emplace_back(cell);

        x += cell.cellwidth;
        row.size += size;
    }
}

void
TermOverlay::addDoubleLine(const QString &text)
{
    addLine(text).flags |= Tsq::DblTopLine;
    addLine(text).flags |= Tsq::DblBottomLine;
}
