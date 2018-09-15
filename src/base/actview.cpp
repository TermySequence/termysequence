// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "manager.h"
#include "listener.h"
#include "stack.h"
#include "term.h"
#include "scrollport.h"
#include "mainwindow.h"
#include "contentmodel.h"
#include "settings/settings.h"
#include "settings/profile.h"
#include "settings/theme.h"
#include "settings/termlayout.h"
#include "settings/layoutadjust.h"
#include "settings/colordialog.h"
#include "settings/fontdialog.h"

#include <random>

static std::default_random_engine s_randomgen;
static std::uniform_int_distribution<uint32_t> s_randomdist;

void
TermManager::actionNextPane()
{
    int pos = m_stack ? m_stack->pos() : 0, n = m_stacks.size();
    emit m_stacks[(pos + 1) % n]->focusRequest();
}

void
TermManager::actionPreviousPane()
{
    int pos = m_stack ? m_stack->pos() : 0, n = m_stacks.size();
    emit m_stacks[(pos + n - 1) % n]->focusRequest();
}

void
TermManager::actionSwitchPane(QString index)
{
    int pos = index.toInt();
    if (pos >= 0 && pos < m_stacks.size())
        emit m_stacks[pos]->focusRequest();
}

void
TermManager::actionSplitViewHorizontal()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_HRESIZE);
}

void
TermManager::actionSplitViewHorizontalFixed()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_HFIXED);
}

void
TermManager::actionSplitViewVertical()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_VRESIZE);
}

void
TermManager::actionSplitViewVerticalFixed()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_VFIXED);
}

void
TermManager::actionSplitViewQuadFixed()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_QFIXED);
}

void
TermManager::actionSplitViewClose()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_CLOSE);
}

void
TermManager::actionSplitViewCloseOthers()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_CLOSEOTHERS);
}

void
TermManager::actionSplitViewExpand()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_EXPAND);
}

void
TermManager::actionSplitViewShrink()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_SHRINK);
}

void
TermManager::actionSplitViewEqualize()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_EQUALIZE);
}

void
TermManager::actionSplitViewEqualizeAll()
{
    if (m_stack)
        emit m_stack->splitRequest(SPLITREQ_EQUALIZEALL);
}

void
TermManager::actionScrollLineUp()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_LINEUP);
}

void
TermManager::actionScrollPageUp()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_PAGEUP);
}

void
TermManager::actionScrollToTop()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_TOP);
}

void
TermManager::actionScrollLineDown()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_LINEDOWN);
}

void
TermManager::actionScrollPageDown()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_PAGEDOWN);
}

void
TermManager::actionScrollToBottom()
{
    if (m_scrollport)
        emit m_scrollport->scrollRequest(SCROLLREQ_BOTTOM);
}

void
TermManager::actionScrollRegionStart(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->scrollToRegion(id, true, true);
}

void
TermManager::actionScrollRegionEnd(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->scrollToRegion(id, false, true);
}

void
TermManager::actionScrollRegionRelative(QString regionId, QString offset)
{
    if (m_scrollport)
        m_scrollport->scrollRelativeToRegion(regionId.toUInt(), offset.toInt());
}

void
TermManager::actionScrollSemantic(QString regionId)
{
    if (m_scrollport)
        m_scrollport->scrollToSemantic(regionId.toUInt());
}

void
TermManager::actionScrollSemanticRelative(QString regionId, QString offset)
{
    if (m_scrollport)
        m_scrollport->scrollRelativeToSemantic(regionId.toUInt(), offset.toInt());
}

void
TermManager::actionScrollImage(QString terminalId, QString contentId)
{
    TermInstance *result = lookupTerm(terminalId);
    const TermContent *content;

    if (result && m_scrollport && m_scrollport->term() == result)
        if ((content = result->content()->content(contentId)))
            m_scrollport->scrollToRow(content->row, true);
}

void
TermManager::actionHighlightCursor()
{
    if (m_term)
        emit m_term->cursorHighlight();
}

void
TermManager::actionHighlightSemanticRegions()
{
    if (m_scrollport)
        m_scrollport->highlightSemantic();
}

void
TermManager::actionToggleFullScreen()
{
    m_parent->setFullScreenMode(!m_parent->isFullScreen());
}

void
TermManager::actionExitFullScreen()
{
    m_parent->setFullScreenMode(false);
}

void
TermManager::actionTogglePresentationMode()
{
    g_listener->setPresMode(!g_listener->presMode());
}

void
TermManager::actionExitPresentationMode()
{
    g_listener->setPresMode(false);
}

void
TermManager::actionToggleMenuBar()
{
    m_parent->setMenuBarVisible(!m_parent->menuBarVisible());
}

void
TermManager::actionShowMenuBar()
{
    m_parent->setMenuBarVisible(true);
}

void
TermManager::actionToggleStatusBar()
{
    m_parent->setStatusBarVisible(!m_parent->statusBarVisible());
}

void
TermManager::actionToggleTerminalLayout(QString itemStr, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    int item = itemStr.toInt();
    if (result && item > 0 && item < LAYOUT_N_WIDGETS) {
        TermLayout layout(result->layout());
        layout.toggleEnabled(layout.itemPosition(item));
        result->setLayout(layout.layoutStr());
    }
}

bool
TermManager::raiseAdjustDialog(TermInstance *term, const QMetaObject &metaObj)
{
    const char *className = metaObj.className();

    for (QObject *child: m_parent->children()) {
        if (child->inherits(className)) {
            AdjustDialog *dialog = static_cast<AdjustDialog*>(child);
            if (dialog->term() == term) {
                dialog->bringUp();
            } else {
                dialog->deleteLater();
            }
            return true;
        }
    }
    return false;
}

void
TermManager::actionAdjustTerminalLayout(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result && !raiseAdjustDialog(result, LayoutAdjust::staticMetaObject)) {
        auto *dialog = new LayoutAdjust(result, this, m_parent);
        dialog->show();
    }
}

void
TermManager::actionAdjustTerminalFont(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result && !raiseAdjustDialog(result, FontDialog::staticMetaObject)) {
        auto *dialog = new FontDialog(result, this, m_parent);
        dialog->show();
    }
}

void
TermManager::actionAdjustTerminalColors(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result && !raiseAdjustDialog(result, ColorDialog::staticMetaObject)) {
        auto *dialog = new ColorDialog(result, this, m_parent);
        dialog->show();
    }
}

void
TermManager::actionRandomTerminalTheme(QString sameGroup, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    auto candidates = g_settings->themes();
    ThemeSettings *curTheme = nullptr;

    if (!result)
        return;

    for (auto *theme: candidates)
        if (theme->content() == result->palette()) {
            curTheme = theme;
            break;
        }
    if (curTheme) {
        if (sameGroup.toInt() == 1) {
            candidates = g_settings->groupThemes(curTheme->group());
        }
        candidates.removeOne(curTheme);
    }
    if (!candidates.isEmpty()) {
        int rando = s_randomdist(s_randomgen) % candidates.size();
        result->setPalette(candidates.at(rando)->content());
    }
}

void
TermManager::actionIncreaseFont()
{
    for (auto term: qAsConst(m_terms)) {
        QFont font = term->realFont();
        font.setPointSize(font.pointSize() + 1);
        term->setFont(font);
    }
}

void
TermManager::actionDecreaseFont()
{
    for (auto term: qAsConst(m_terms)) {
        QFont font = term->realFont();
        if (font.pointSize() > 1) {
            font.setPointSize(font.pointSize() - 1);
            term->setFont(font);
        }
    }
}

void
TermManager::actionUndoTerminalAdjustments(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        const ProfileSettings *profile = result->profile();
        QFont font;
        font.fromString(profile->font());
        result->setFont(font);
        result->setPalette(profile->content());
        result->setLayout(profile->layout());
        result->setBadge(profile->badge());
        result->setIcon(profile->icon());
    }
}

void
TermManager::actionUndoAllAdjustments()
{
    for (auto term: qAsConst(m_terms)) {
        const ProfileSettings *profile = term->profile();
        QFont font;
        font.fromString(profile->font());
        term->setFont(font);
        term->setPalette(profile->content());
        term->setLayout(profile->layout());
        term->setBadge(profile->badge());
        term->setIcon(profile->icon());
    }
}

void
TermManager::actionToggleTerminalFollowing()
{
    if (m_scrollport)
        m_scrollport->setFollowable(!m_scrollport->followable());
}

void
TermManager::actionTerminalContextMenu()
{
    if (m_scrollport)
        emit m_scrollport->contextMenuRequest();
}
