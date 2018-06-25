// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/customaction.h"
#include "app/logging.h"
#include "manager.h"
#include "listener.h"
#include "server.h"
#include "stack.h"
#include "term.h"
#include "scrollport.h"
#include "mainwindow.h"
#include "statusarrange.h"
#include "termformat.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/keymap.h"

#include <QTimer>
#include <QMetaMethod>

#define TR_TEXT1 TL("window-text", "Waiting for next keystroke of shortcut - ESC to cancel")
#define TR_TEXT2 TL("window-text", "Unrecognized shortcut")
#define TR_TEXT3 TL("window-text", "Shortcut canceled")
#define TR_TEXT4 TL("window-text", "Wrote %n characters to %1", "", n)
#define TR_TEXT5 TL("window-text", "Wrote %n bytes to %1", "", n)
#define TR_TEXT6 TL("window-text", "Wrote image data to %1")
#define TR_TEXT7 TL("window-text", "clipboard")
#define TR_TEXT8 TL("window-text", "selection buffer")
#define TR_TEXT9 TL("window-text", "Remote")

TermManager::TermManager() :
    m_server(g_listener->localServer())
{
    connect(g_listener, SIGNAL(serverAdded(ServerInstance*)), SLOT(handleServerAdded(ServerInstance*)));
    connect(g_listener, SIGNAL(serverRemoved(ServerInstance*)), SLOT(handleServerRemoved(ServerInstance*)));
    connect(g_listener, SIGNAL(termAdded(TermInstance*)), SLOT(handleTermAdded(TermInstance*)));
    connect(g_listener, SIGNAL(termRemoved(TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));
    connect(this, &TermManager::invokeRequest, this, &TermManager::invokeSlot, Qt::QueuedConnection);

    QTimer::singleShot(g_global->populateTime(), this, SLOT(timerCallback()));
}

void
TermManager::setParent(MainWindow *parent)
{
    QObject::setParent(parent);
    m_parent = parent;
}

void
TermManager::timerCallback()
{
    m_populating = false;
    emit populated();
}

//
// Non-slot methods
//
void
TermManager::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activeChanged(active);
    }

    // Fix for Alt focus bug on Ubuntu
    if (active && m_stack) {
        emit m_stacks[m_stack->pos()]->focusRequest();
    }
}

void
TermManager::raiseTerm(TermInstance *term)
{
    if (m_stack) {
        m_stack->setTerm(term);
        raiseTerminalsWidget(false);
    }
}

void
TermManager::setActiveServer(ServerInstance *server)
{
    if (m_server != server)
        emit serverActivated(m_server = server);
}

void
TermManager::setActiveTerm(TermInstance *term, TermScrollport *scrollport)
{
    if (m_term != term || m_scrollport != scrollport) {
        disconnect(m_mocTitle);

        if (term) {
            m_mocTitle = connect(term->format(), SIGNAL(titleChanged(const QString&)),
                                 SLOT(handleTitle(const QString&)));
            m_title = term->format()->title();
            setActiveServer(term->server());
        } else {
            m_title.clear();
        }

        emit titleChanged(m_title);
        emit termActivated(m_term = term, m_scrollport = scrollport);
    }
}

void
TermManager::handleTitle(const QString &title)
{
    emit titleChanged(m_title = title);
}

void
TermManager::setActiveStack(TermStack *stack)
{
    if (m_stack != stack) {
        m_stack = stack;
    }
}

void
TermManager::addStack(TermStack *stack, int pos)
{
    m_stacks.insert(pos, stack);
    connect(stack, SIGNAL(destroyed(QObject*)), SLOT(handleStackDestroyed(QObject*)));

    emit stackReordered();

    if (m_stack == nullptr)
        setActiveStack(stack);
}

void
TermManager::handleStackDestroyed(QObject *object)
{
    TermStack *stack = static_cast<TermStack*>(object);

    if (!m_stacks.removeOne(stack))
        return;

    if (m_stack == stack)
        setActiveStack(nullptr);

    if (m_stacks.isEmpty())
        m_parent->close();
    else
        emit stackReordered();
}

void
TermManager::handleServerAdded(ServerInstance *server)
{
    TermOrder::addServer(server);

    if (m_server == nullptr)
        setActiveServer(server);
}

void
TermManager::handleServerRemoved(ServerInstance *server)
{
    if (m_server == server)
        setActiveServer(g_listener->localServer());

    TermOrder::removeServer(server);
}

void
TermManager::handleTermAdded(TermInstance *term)
{
    TermOrder::addTerm(term);

    bool changed = false;

    for (auto stack: qAsConst(m_stacks))
        if (stack->term() == nullptr) {
            stack->setTerm(term);
            changed = true;
        }

    if (changed)
        raiseTerminalsWidget(false);
}

void
TermManager::handleTermRemoved(TermInstance *term)
{
    bool changing = m_term == term;

    TermOrder::removeTerm(term);

    if (m_term == term)
        setActiveTerm(nullptr, nullptr);

    if (isEmpty() && g_global->autoQuit())
        emit finished();
    else if (changing)
        raiseTerminalsWidget(m_term);
}

TermInstance *
TermManager::lookupTerm(const QString &terminalId)
{
    Tsq::Uuid termId;

    if (termId.parse(terminalId.toLatin1().data()))
        return m_termMap.value(termId);
    else
        return m_term;
}

ServerInstance *
TermManager::lookupServer(const QString &serverId)
{
    Tsq::Uuid servId;

    if (serverId.isEmpty())
        return m_server;
    else if (!servId.parse(serverId.toLatin1().data()))
        return g_listener->localServer();
    else
        return m_serverMap.value(servId, g_listener->localServer());
}

void
TermManager::saveOrder(OrderLayoutWriter &layout) const
{
    for (auto server: qAsConst(m_servers))
        layout.addServer(server->id());
    for (auto term: qAsConst(m_terms))
        layout.addTerm(term->id(), isHidden(term));
}

void
TermManager::loadOrder(OrderLayout &layout)
{
    m_layout = std::move(layout);

    sortAll();

    for (auto term: qAsConst(m_terms))
        if (m_layout.hidden(term->id()))
            setHidden(term, true);

    if (isHidden(m_term))
        actionNextTerminal();
}

void
TermManager::raiseTerminalsWidget(bool peek)
{
    if (g_global->raiseTerminals())
        m_parent->popDockWidget(m_parent->terminalsDock());
    if (peek & g_global->showPeek())
        m_stack->showPeek();
}

void
TermManager::reportClipboardCopy(int n, int type, bool selbuf, bool remote)
{
    QString msg;
    QString dest = selbuf ? TR_TEXT8 : TR_TEXT7;

    switch (type) {
    case CopiedBytes:
        msg = TR_TEXT5.arg(dest);
        break;
    case CopiedImage:
        msg = TR_TEXT6.arg(dest);
        break;
    default:
        msg = TR_TEXT4.arg(dest);
    }

    if (remote)
        msg += L(" (%1)").arg(TR_TEXT9);

    m_parent->status()->showMinor(msg);
}

//
// Slot metaprogramming
//
#define QA(i) Q_ARG(QString, args.value(i))

QHash<QByteArray,int> TermManager::s_actions;
QStringList TermManager::s_actionSignatures;
QStringList TermManager::s_termSignatures;
QStringList TermManager::s_serverSignatures;

void
TermManager::initialize()
{
    const QMetaObject &o = TermManager::staticMetaObject;
    const QByteArray ignoreTerm = B("terminalId");
    const QByteArray ignoreServ = B("serverId");
    const QByteArray ignorePath = B("filePath");

    for (int i = o.methodOffset(); i < o.methodCount(); ++i)
    {
        QByteArray slot = o.method(i).name();
        if (!slot.startsWith(ACTION_PREFIX))
            continue;

        const QList<QByteArray> params = o.method(i).parameterNames();
        slot.remove(0, ACTION_PREFIX_LEN);
        s_actions[slot] = params.size();
        s_actionSignatures.append(slot);

        bool ignored = false;

        for (auto &p: params) {
            if (p == ignoreTerm || p == ignoreServ || p == ignorePath)
                ignored = true;

            slot += A("|<") + p + '>';

            if (!ignored)
                s_actionSignatures.append(slot);
        }

        if (slot.contains("<serverId>")) {
            s_termSignatures.append(slot);
            s_serverSignatures.append(slot);
        } else if (slot.contains("<terminalId>")) {
            s_termSignatures.append(slot);
        }
    }

    s_actionSignatures.sort();
    s_termSignatures.sort();
    s_serverSignatures.sort();
}

void
TermManager::invokeSlot(const QString &slot, bool fromKeyboard)
{
    if (fromKeyboard) {
        m_parent->status()->showMinor(slot);
        emit invoking(slot);
    }

    QStringList args = slot.split('|');
    QByteArray actionName = args.takeFirst().toLatin1();

    if (actionName.startsWith(CUSTOM_PREFIX)) {
        actionName.remove(0, CUSTOM_PREFIX_LEN);
        ActionFeature::invoke(this, actionName, args);
        return;
    }
    auto i = s_actions.constFind(actionName);
    if (i == s_actions.cend()) {
        qCWarning(lcSettings) << "Unknown action" << actionName << "invoked";
        return;
    }

    actionName.prepend(ACTION_PREFIX);
    const char *slotChars = actionName.constData();

    switch (*i) {
    case 0:
        QMetaObject::invokeMethod(this, slotChars, Qt::DirectConnection);
        break;
    case 1:
        QMetaObject::invokeMethod(this, slotChars, Qt::DirectConnection, QA(0));
        break;
    case 2:
        QMetaObject::invokeMethod(this, slotChars, Qt::DirectConnection, QA(0), QA(1));
        break;
    case 3:
        QMetaObject::invokeMethod(this, slotChars, Qt::DirectConnection, QA(0), QA(1), QA(2));
        break;
    case 4:
        QMetaObject::invokeMethod(this, slotChars, Qt::DirectConnection, QA(0), QA(1), QA(2), QA(3));
        break;
    }
}

bool
TermManager::validateSlot(const QByteArray &candidate)
{
    int idx = candidate.indexOf('|');
    QByteArray name = (idx == -1) ? candidate : candidate.left(idx);

    return s_actions.contains(name) || name.startsWith(CUSTOM_PREFIX);
}

bool
TermManager::lookupShortcut(QString &slot, QString &keymap, TermShortcut *result)
{
    bool retval;
    TermKeymap *k;
    Tsq::TermFlags flags;

    if (m_stack) {
        TermInstance *term = m_stack->term();
        k = term ? term->profile()->keymap() : g_settings->defaultKeymap();
        flags = term ? term->flags() : Tsq::DefaultTermFlags;
    } else {
        k = g_settings->defaultKeymap();
        flags = Tsq::DefaultTermFlags;
    }

    keymap = k->name();
    retval = k->lookupShortcut(slot, flags, result);
    return retval;
}

void
TermManager::reportComboStarted(const TermKeymap *keymap)
{
    m_mocCombo = connect(keymap, SIGNAL(comboFinished(int)), SLOT(handleComboFinished(int)));
    m_parent->status()->showPermanent(TR_TEXT1);
}

void
TermManager::handleComboFinished(int code)
{
    disconnect(m_mocCombo);
    m_parent->status()->clearPermanent();

    switch (code) {
    case COMBO_FAIL:
        m_parent->status()->showMajor(TR_TEXT2);
        break;
    case COMBO_CANCEL:
    case COMBO_RESET:
        m_parent->status()->showMajor(TR_TEXT3);
        break;
    default:
        break;
    }
}
