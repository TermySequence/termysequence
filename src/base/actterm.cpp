// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logging.h"
#include "manager.h"
#include "listener.h"
#include "conn.h"
#include "stack.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "mainwindow.h"
#include "infowindow.h"
#include "portouttask.h"
#include "portintask.h"
#include "thumbicon.h"
#include "settings/settings.h"
#include "settings/connect.h"
#include "settings/powerdialog.h"
#include "settings/sshdialog.h"
#include "settings/userdialog.h"
#include "settings/containerdialog.h"
#include "settings/otherdialog.h"
#include "settings/imagedialog.h"

void
TermManager::actionFirstTerminal()
{
    TermInstance *result = firstTerm();
    if (m_stack) {
        m_stack->setTerm(result);
        raiseTerminalsWidget(true);
    }
}

void
TermManager::actionPreviousTerminal()
{
    TermInstance *result = prevTerm(m_term);
    if (m_stack) {
        m_stack->setTerm(result);
        raiseTerminalsWidget(true);
    }
}

void
TermManager::actionNextTerminal()
{
    TermInstance *result = nextTerm(m_term);
    if (m_stack) {
        m_stack->setTerm(result);
        raiseTerminalsWidget(true);
    }
}

void
TermManager::actionSwitchTerminal(QString terminalId, QString index)
{
    TermInstance *result = lookupTerm(terminalId);
    auto *stack = index.isEmpty() ? m_stack : m_stacks.value(index.toUInt());

    if (stack && result) {
        setHidden(result, false);
        raiseTerminalsWidget(stack->setTerm(result));
    }
}

void
TermManager::actionHideTerminal(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result)
        setHidden(result, true);
    if (isHidden(m_term))
        actionNextTerminal();
}

void
TermManager::actionHideTerminalEverywhere(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result)
        for (auto manager: g_listener->managers())
            manager->actionHideTerminal(result->idStr());
}

void
TermManager::actionShowTerminal(QString terminalId)
{
    if (terminalId.isEmpty()) {
        for (auto term: qAsConst(m_terms))
            setHidden(term, false);
    }
    else {
        TermInstance *result = lookupTerm(terminalId);
        if (result)
            setHidden(result, false);
    }

    if (!m_term)
        actionFirstTerminal();
}

void
TermManager::actionReorderTerminalForward(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        reorderTermForward(result);
}

void
TermManager::actionReorderTerminalBackward(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        reorderTermBackward(result);
}

void
TermManager::actionReorderTerminalFirst(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        reorderTermFirst(result);
}

void
TermManager::actionReorderTerminalLast(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        reorderTermLast(result);
}

void
TermManager::actionClearTerminalScrollback(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermReset(result, Tsq::ClearScrollback);
}

void
TermManager::actionAdjustTerminalScrollback(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        TermBuffers *buffers = result->buffers();
        auto *dialog = new PowerDialog(result, this, m_parent);
        dialog->setValue(buffers->caporder());
        dialog->show();
    }
}

void
TermManager::actionResetTerminal(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermReset(result, Tsq::ResetEmulator);
}

void
TermManager::actionResetAndClearTerminal(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermReset(result, Tsq::ResetEmulator|Tsq::ClearScrollback|
                                  Tsq::ClearScreen|Tsq::FormFeed);
}

void
TermManager::actionSetTerminalIcon(QString icon, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        if (icon == g_str_PROMPT_PROFILE) {
            auto *dialog = new ImageDialog(ThumbIcon::TerminalType, result, m_parent);
            connect(dialog, SIGNAL(okayed(QString,QString)),
                    SLOT(actionSetTerminalIcon(QString,QString)));
            dialog->show();
        } else {
            result->setIcon(icon);
            ThumbIcon::setFavoriteIcon(ThumbIcon::TerminalType, icon);
        }
    }
}

void
TermManager::actionSetServerIcon(QString icon, QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result) {
        if (icon == g_str_PROMPT_PROFILE) {
            auto *dialog = new ImageDialog(ThumbIcon::ServerType, result, m_parent);
            connect(dialog, SIGNAL(okayed(QString,QString)),
                    SLOT(actionSetServerIcon(QString,QString)));
            dialog->show();
        } else {
            result->setIcon(icon, true);
            ThumbIcon::setFavoriteIcon(ThumbIcon::ServerType, icon);
        }
    }
}

void
TermManager::actionSendSignal(QString signal, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermSignal(result, signal.toUInt());
}

void
TermManager::actionTakeTerminalOwnership(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        result->takeOwnership();
}

void
TermManager::actionToggleTerminalRemoteInput(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        result->setRemoteInput(!result->remoteInput());
}

void
TermManager::actionToggleSoftScrollLock(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermScrollLock(result);
}

void
TermManager::actionTimingSetOriginContext()
{
    if (m_scrollport)
        m_scrollport->setTimingOriginByClickPoint();
}

void
TermManager::actionTimingFloatOrigin()
{
    if (m_scrollport)
        m_scrollport->floatTimingOrigin();
}

void
TermManager::actionInputSetLeader(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->setInputLeader(result);
}

void
TermManager::actionInputUnsetLeader()
{
    g_listener->unsetInputLeader();
}

void
TermManager::actionInputSetFollower(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->setInputFollower(result);
}

void
TermManager::actionInputUnsetFollower(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->unsetInputFollower(result);
}

void
TermManager::actionInputToggleFollower(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        if (result->inputFollower())
            g_listener->unsetInputFollower(result);
        else
            g_listener->setInputFollower(result);
    }
}

void
TermManager::actionPreviousServer()
{
    ServerInstance *result = prevServer(m_server);
    setActiveServer(result);
    if (m_stack && result) {
        m_stack->setTerm(nextTermWithin(m_term, result));
        raiseTerminalsWidget(true);
    }
}

void
TermManager::actionNextServer()
{
    ServerInstance *result = nextServer(m_server);
    setActiveServer(result);
    if (m_stack && result) {
        m_stack->setTerm(nextTermWithin(m_term, result));
        raiseTerminalsWidget(true);
    }
}

void
TermManager::actionSwitchServer(QString serverId, QString index)
{
    ServerInstance *result = lookupServer(serverId);
    setActiveServer(result);

    auto *stack = index.isEmpty() ? m_stack : m_stacks.value(index.toUInt());
    if (stack && m_term && m_term->server() != result) {
        bool rc = stack->setTerm(nextTermWithin(m_term, result));
        raiseTerminalsWidget(rc);
    }
}

void
TermManager::actionHideServer(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result) {
        setHidden(result, true);

        if (isHidden(m_term))
            actionNextTerminal();
    }
}

void
TermManager::actionShowServer(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result) {
        setHidden(result, false);

        if (!m_term)
            actionFirstTerminal();
    }
}

void
TermManager::actionToggleServer(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result) {
        if (hiddenCount(result)) {
            actionShowServer(serverId);
        } else {
            actionHideServer(serverId);
        }
    }
}

void
TermManager::actionReorderServerForward(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        reorderServerForward(result);
}

void
TermManager::actionReorderServerBackward(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        reorderServerBackward(result);
}

void
TermManager::actionReorderServerFirst(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        reorderServerFirst(result);
}

void
TermManager::actionReorderServerLast(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        reorderServerLast(result);
}

void
TermManager::actionViewTerminalInfo(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        InfoWindow::showTermWindow(result);
}

void
TermManager::actionViewTerminalContent(QString contentId, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        InfoWindow::showContentWindow(result, contentId);
}

void
TermManager::actionViewServerInfo(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        InfoWindow::showServerWindow(result);
}

void
TermManager::actionDisconnectTerminal(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        g_listener->pushTermDisconnect(result);
}

void
TermManager::actionDisconnectServer(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        result->unconnect();
}

void
TermManager::actionOpenConnection(QString connName)
{
    ConnectSettings *conn = g_settings->conn(connName);
    if (conn)
        g_listener->launchConnection(conn, this);
}

void
TermManager::actionNewConnection(QString connType, QString connArg, QString serverId)
{
    int type = connType.toInt();

    if (connArg.isEmpty()) {
        ConnectDialog *dialog;

        switch (type) {
        case Tsqt::ConnectionSsh:
            dialog = new SshDialog(m_parent);
            break;
        case Tsqt::ConnectionUserSudo:
        case Tsqt::ConnectionUserSu:
        case Tsqt::ConnectionUserMctl:
        case Tsqt::ConnectionUserPkexec:
            dialog = new UserDialog(m_parent, type);
            break;
        case Tsqt::ConnectionMctl:
        case Tsqt::ConnectionDocker:
        case Tsqt::ConnectionKubectl:
        case Tsqt::ConnectionRkt:
            dialog = new ContainerDialog(m_parent, type);
            break;
        default:
            dialog = new OtherDialog(m_parent);
        }
        connect(dialog, &QDialog::accepted, [=]{
            g_listener->launchConnection(dialog->conn(), this);
        });
        dialog->show();
    }
    else {
        ConnectSettings *conn = nullptr;

        switch (type) {
        case Tsqt::ConnectionSsh:
            conn = SshDialog::makeConnection(connArg);
            break;
        case Tsqt::ConnectionUserSudo:
        case Tsqt::ConnectionUserSu:
        case Tsqt::ConnectionUserMctl:
        case Tsqt::ConnectionUserPkexec:
            conn = UserDialog::makeConnection(type, connArg);
            break;
        case Tsqt::ConnectionMctl:
        case Tsqt::ConnectionDocker:
        case Tsqt::ConnectionKubectl:
        case Tsqt::ConnectionRkt:
            conn = ContainerDialog::makeConnection(type, connArg);
            break;
        default:
            return;
        }
        conn->setServer(serverId);
        g_listener->launchConnection(conn, this);
        conn->putReference();
    }
}

void
TermManager::actionLocalPortForward(QString serverId, QString spec)
{
    PortFwdRule config;
    if (!config.parseSpec(spec)) {
        qCWarning(lcSettings, "Invalid port forwarding spec '%s'", pr(spec));
        return;
    }
    config.islocal = true;

    ServerInstance *result = lookupServer(serverId);
    if (result)
        (new PortOutTask(result, config))->start(this);
}

void
TermManager::actionRemotePortForward(QString serverId, QString spec)
{
    PortFwdRule config;
    if (!config.parseSpec(spec)) {
        qCWarning(lcSettings, "Invalid port forwarding spec '%s'", pr(spec));
        return;
    }
    config.islocal = false;

    ServerInstance *result = lookupServer(serverId);
    if (result)
        (new PortInTask(result, config))->start(this);
}

void
TermManager::actionSendMonitorInput(QString serverId, QString message)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        g_listener->pushServerMonitorInput(result, message.toUtf8());
}
