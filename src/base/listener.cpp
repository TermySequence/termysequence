// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "app/reaper.h"
#include "listener.h"
#include "conn.h"
#include "reader.h"
#include "manager.h"
#include "stack.h"
#include "term.h"
#include "blinktimer.h"
#include "effecttimer.h"
#include "scrolltimer.h"
#include "fetchtimer.h"
#include "jobmodel.h"
#include "notemodel.h"
#include "taskmodel.h"
#include "connecttask.h"
#include "portouttask.h"
#include "portintask.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/servinfo.h"
#include "settings/connect.h"
#include "settings/splitlayout.h"
#include "settings/port.h"
#include "setup/systemdsetup.h"
#include "os/time.h"
#include "os/attr.h"
#include "os/dir.h"
#include "os/conn.h"

#include <QSocketNotifier>
#include <exception>
#include <unistd.h>

#define TR_ERROR1 TL("error", "Connection %1 failed")
#define TR_ERROR3 TL("error", "Lost connection to %1")
#define TR_TEXT1 TL("window-text", "Make sure %1 executable is installed and up to date")
#define TR_TEXT2 TL("window-text", "Visit %1 for help")
#define TR_TEXT3 TL("window-text", "Was the server killed?")
#define TR_TITLE1 TL("window-title", "Failed to Connect")
#define TR_TITLE2 TL("window-title", "Lost Connection")

TermListener *g_listener;
static bool s_pluginPrompt;

TermListener::TermListener() :
    m_jobmodel(new JobModel(this)),
    m_notemodel(new NoteModel(this)),
    m_taskmodel(new TaskModel(this)),
    m_blink(new TermBlinkTimer(this)),
    m_effects(new TermEffectTimer(this)),
    m_scroll(new TermScrollTimer(this)),
    m_fetch(new TermFetchTimer(this)),
    m_commandMode(g_global->commandMode()),
    m_flags(m_commandMode ? Tsqt::CommandMode : 0)
{
    m_idStr = QString::fromLatin1(m_id.str().c_str());

    std::vector<int> pids;
    int64_t before = osWalltime();
    osAttributes(m_attributes, pids, false);
    int64_t after = osWalltime();
    for (int pid: pids)
        ReaperThread::launchReaper(pid);

    m_attributes[TSQ_ATTR_STARTED] = std::to_string(after);
    m_attributes[TSQ_ATTR_PRODUCT] = APP_NAME " " PROJECT_VERSION;
    m_attributes[TSQ_ATTR_AVATAR] = g_settings->loadAvatar(m_idStr);

    if (after - before > ATTRIBUTE_SCRIPT_WARN)
        qCWarning(lcSettings, "Warning: slow identity or attribute scripts "
                  "delayed startup by %" PRId64 "ms", after - before);

    connect(g_global, &GlobalSettings::preferTransientChanged,
            this, &TermListener::recheckLocalServer);
}

void
TermListener::setCommandMode(bool commandMode)
{
    if (m_commandMode != commandMode) {
        if (commandMode)
            m_flags |= Tsqt::CommandMode;
        else
            m_flags &= ~Tsqt::CommandMode;

        emit commandModeChanged(m_commandMode = commandMode);
        emit flagsChanged(m_flags);
    }
}

void
TermListener::setPresMode(bool presMode)
{
    if (m_presMode != presMode)
        emit presModeChanged(m_presMode = presMode);
}

void
TermListener::quit()
{
    for (auto manager: m_managers) {
        manager->disconnect(this);
        manager->parentWidget()->close();
    }

    m_manager = nullptr;
    m_managers.clear();
}

inline void
TermListener::hookupInitial(ServerConnection *conn)
{
    connect(conn, SIGNAL(ready()), SLOT(handleInitialReady()));
    connect(conn, SIGNAL(connectionFailed()), SLOT(handleInitialReady()));
}

inline void
TermListener::hookup(ServerConnection *conn)
{
    connect(conn, SIGNAL(ready()), SLOT(handleConnectionReady()));
    connect(conn, SIGNAL(connectionFailed()), SLOT(handleConnectionFailed()));
}

void
TermListener::start()
{
    if (!g_global->launchTransient() && !g_global->launchPersistent()) {
        emit ready();
        goto skip;
    }
    if (g_global->launchTransient()) {
        m_transient = new ServerConnection(g_settings->transientConn());
        hookupInitial(m_transient);
        m_transient->start(true);
    }
    if (g_global->launchPersistent()) {
        m_persistent = new ServerConnection(g_settings->persistentConn());
        hookupInitial(m_persistent);
        m_persistent->start(false);
    }
skip:
    try {
        QString adir = qApp->property(OBJPROP_ADIR).toString();
        char udspath[SOCKET_PATHLEN];
        osCreateRuntimeDir(pr(adir), udspath);
        osCreateSocketPath(udspath);
        int fd = osLocalListen(udspath);
        m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_notifier, SIGNAL(activated(int)), SLOT(handleAccept(int)));
    }
    catch (const std::exception &e) {
        qCWarning(lcSettings, "Warning: failed to listen on local socket: %s", e.what());
    }
}

QObject *
TermListener::launchConnection(ConnectSettings *info, TermManager *manager)
{
    if (info->name() == g_str_PERSISTENT_CONN) {
        if (!m_persistent) {
            m_persistent = new ServerConnection(g_settings->persistentConn());
            hookup(m_persistent);
            m_persistent->start(false);
        }
        return m_persistent;
    }
    else if (info->name() == g_str_TRANSIENT_CONN) {
        if (!m_transient) {
            m_transient = new ServerConnection(g_settings->transientConn());
            hookup(m_transient);
            m_transient->start(true);
        }
        return m_transient;
    }
    else if (info->isbatch()) {
        auto *task = new ConnectBatch(info);
        task->start(manager);
        return task;
    }
    else if (info->server().isEmpty()) {
        auto *conn = new ServerConnection(info);
        hookup(conn);
        conn->start(manager->parentWidget());
        return conn;
    }
    else {
        auto *task = new ConnectTask(lookupServer(info->launchId()), info);
        task->start(manager);
        return task;
    }
}

void
TermListener::addServer(ServerInstance *server)
{
    m_servers.insert(server->id(), server);
    m_servlist.append(server);

    if (server->conn() != server)
        connect(server, SIGNAL(ready()), SLOT(handleServerReady()));

    emit serverAdded(server);
}

void
TermListener::removeServer(ServerInstance *server)
{
    m_servers.remove(server->id());
    m_servlist.removeOne(server);
    emit serverRemoved(server);
}

void
TermListener::addTerm(TermInstance *term)
{
    m_terms.insert(term->id(), term);
    emit termAdded(term);
}

void
TermListener::removeTerm(TermInstance *term)
{
    if (m_inputLeader) {
        if (term == m_inputLeader)
            unsetInputLeader();
        else
            m_inputFollowers.remove(term);
    }

    m_terms.remove(term->id());
    emit termRemoved(term);
}

void
TermListener::addManager(TermManager *manager)
{
    m_managers.append(manager);
    connect(manager, SIGNAL(destroyed(QObject*)), SLOT(handleManagerDestroyed(QObject*)));
    connect(manager, SIGNAL(stackReordered()), SLOT(handleStackReordered()));
    emit managerAdded(manager);
}

void
TermListener::launchStartupTerminals(ServerInstance *server)
{
    QStringList profiles = server->serverInfo()->startup();
    QHash<ProfileSettings*,int> profileSet;

    for (auto term: server->terms())
        ++profileSet[term->profile()];

    // Complete the startup group
    for (auto &&i: profiles) {
        ProfileSettings *profile = server->serverInfo()->profile(i);
        i = profile->name();

        if (profileSet[profile])
            --profileSet[profile];
        else
            m_manager->createTerm(server, profile);
    }

    // Sort by startup group
    if (!profiles.isEmpty()) {
        auto *first = m_manager->sortTermsByProfile(server, profiles);
        if (server->ours() && first)
            m_manager->raiseTerm(first);

        for (auto manager: qAsConst(m_managers))
            if (manager != m_manager)
                manager->sortTermsByProfile(server, profiles);
    }

    // Launch automatic port forwarding tasks
    for (auto &i: PortFwdList(server->serverInfo()->ports()))
        if (i.isauto) {
            auto *task = i.islocal ?
                (TermTask*)new PortOutTask(server, i) :
                (TermTask*)new PortInTask(server, i);

            task->disableStatusPopup();
            task->start(m_manager);
        }
}

TermManager *
TermListener::createInitialManager()
{
    TermManager *manager = new TermManager;
    addManager(manager);
    setManager(manager);

    if (m_transient) {
        manager->addServer(m_transient);
        launchStartupTerminals(m_transient);
    }
    if (m_persistent) {
        for (auto server: m_persistent->servers()) {
            manager->addServer(server);
        }
        for (auto term: m_persistent->terms()) {
            manager->addTerm(term);
        }
        launchStartupTerminals(m_persistent);
    }

    return manager;
}

TermInstance *
TermListener::nextAvailableTerm(TermManager *target) const
{
    QSet<TermInstance*> termSet;
    const auto &terms = target->terms();

    if (terms.isEmpty())
        return nullptr;

    // Look for a terminal not active anywhere
    for (auto manager: qAsConst(m_managers))
        for (auto stack: manager->m_stacks)
            termSet.insert(stack->term());

    for (auto term: terms)
        if (!termSet.contains(term) && !target->isHidden(term))
            return term;

    for (auto term: terms)
        if (!termSet.contains(term)) {
            target->setHidden(term, false);
            return term;
        }

    // Look for a terminal not active in the target manager
    termSet.clear();
    for (auto stack: target->m_stacks)
        termSet.insert(stack->term());

    for (auto term: terms)
        if (!termSet.contains(term) && !target->isHidden(term))
            return term;

    for (auto term: terms)
        if (!termSet.contains(term)) {
            target->setHidden(term, false);
            return term;
        }

    // Fallback
    target->setHidden(terms.front(), false);
    return terms.front();
}

TermInstance *
TermListener::findTerm(TermManager *target, const TermState *rec) const
{
    TermInstance *term = target->TermOrder::lookupTerm(rec->termId);

    if (term) {
        target->setHidden(term, false);
        return term;
    }

    QList<TermInstance*> terms = target->terms();

    // Build set of inactive terminals
    for (auto manager: qAsConst(m_managers))
        for (auto stack: manager->m_stacks)
            terms.removeOne(stack->term());

    // Look for a terminal with the given server and profile
    for (auto term: qAsConst(terms))
        if (term->profileName() == rec->profileName &&
            term->server()->id() == rec->serverId && !target->isHidden(term))
            return term;

    for (auto term: qAsConst(terms))
        if (term->profileName() == rec->profileName &&
            term->server()->id() == rec->serverId) {
            target->setHidden(term, false);
            return term;
        }

    // Look for a terminal with the given server
    for (auto term: qAsConst(terms))
        if (term->server()->id() == rec->serverId && !target->isHidden(term))
            return term;

    for (auto term: qAsConst(terms))
        if (term->server()->id() == rec->serverId) {
            target->setHidden(term, false);
            return term;
        }

    // Fallback
    return nextAvailableTerm(target);
}

void
TermListener::handleStackReordered()
{
    int base = 1;

    for (int i = 0; i < m_managers.size(); ++i) {
        const auto &stacks = m_managers.at(i)->m_stacks;

        for (int j = 0; j < stacks.size(); ++j) {
            stacks.at(j)->setIndex(base++);
            stacks.at(j)->setPos(j);
        }
    }
}

void
TermListener::handleManagerDestroyed(QObject *object)
{
    TermManager *manager = static_cast<TermManager*>(object);

    m_managers.removeOne(manager);
    m_activity.removeOne(manager);

    if (m_manager == manager) {
        if (m_activity.isEmpty()) {
            m_manager = nullptr;
        } else {
            m_manager = m_activity.back();
            m_manager->setPrimary(true);
        }
    }

    emit managerRemoved(manager);
    handleStackReordered();
}

void
TermListener::setManager(TermManager *manager)
{
    if (m_manager != manager) {
        if (m_manager)
            m_manager->setPrimary(false);

        m_manager = manager;
        m_activity.removeOne(manager);
        m_activity.append(manager);
        m_manager->setPrimary(true);
    }
}

void
TermListener::recheckLocalServer()
{
    if (m_transient && g_global->preferTransient())
        m_localServer = m_transient;
    else if (m_persistent && g_global->preferTransient())
        m_localServer = m_persistent;
    else
        m_localServer = m_transient ? m_transient : m_persistent;
}

void
TermListener::recheckConnected()
{
    bool connected = false;

    for (auto server: qAsConst(m_servers))
        if (server->conn() != nullptr) {
            connected = true;
            break;
        }

    if (m_connected != connected)
        emit connectedChanged(m_connected = connected);
}

QString
TermListener::makeFailureMessage(QStringList &parts)
{
    QString link = g_global->docLink("failed-to-connect.html", "doc/failed-to-connect.html");

    parts.append(TR_TEXT1.arg(SERVER_NAME));
    parts.append(TR_TEXT2.arg(link));

    return parts.join(A("<br>"));
}

void
TermListener::handleInitialReady()
{
    bool tReady = !m_transient || !m_transient->populating();
    bool tFailed = m_transient && m_transient->failed();
    bool pReady = !m_persistent || !m_persistent->populating();
    bool pFailed = m_persistent && m_persistent->failed();
    bool haveDecision = (tReady || tFailed) && (pReady || pFailed);

    if (!haveDecision) {
        return;
    }

    QStringList parts;

    if (tFailed) {
        parts.append(TR_ERROR1.arg(m_transient->name()) + ':');
        parts.append(m_transient->failureMessage());

        m_transient->deleteLater();
        m_transient = nullptr;
    }
    if (pFailed) {
        parts.append(TR_ERROR1.arg(m_persistent->name()) + ':');
        parts.append(m_persistent->failureMessage());

        m_persistent->deleteLater();
        m_persistent = nullptr;
    }
    if (tFailed || pFailed) {
        m_failureMsg = makeFailureMessage(parts);
        m_failureTitle = TR_TITLE1;
        m_failureType = 1;
    }
    else if (needSystemdSetup(m_persistent, m_transient)) {
        m_failureType = 2; // show setup dialog
    }

    recheckLocalServer();
    recheckConnected();
    emit ready();
}

void
TermListener::handleServerReady()
{
    ServerInstance *server = static_cast<ServerInstance*>(sender());

    if (!m_managers.empty())
        launchStartupTerminals(server);
}

void
TermListener::handleConnectionReady()
{
    ServerInstance *server = static_cast<ServerInstance*>(sender());

    if (!m_managers.empty())
        launchStartupTerminals(server);

    recheckLocalServer();
    recheckConnected();
}

void
TermListener::handleConnectionFailed()
{
    ServerConnection *conn = static_cast<ServerConnection*>(sender());

    if (m_transient == conn)
        m_transient = nullptr;
    else if (m_persistent == conn)
        m_persistent = nullptr;

    if (!conn->dialogShown() && m_manager) {
        auto failureMsg = TR_ERROR1.arg(conn->name()) + A(": ") + conn->failureMessage();

        if (!conn->independent()) {
            QStringList parts(failureMsg);
            failureMsg = makeFailureMessage(parts);
        }

        errBox(TR_TITLE1, failureMsg, m_manager->parentWidget())->show();
    }

    conn->deleteLater();
    recheckLocalServer();
    recheckConnected();
}

void
TermListener::reportConnectionLost(ServerConnection *conn)
{
    if (m_transient == conn)
        m_transient = nullptr;
    else if (m_persistent == conn)
        m_persistent = nullptr;

    if (m_manager) {
        QStringList parts;
        QString link = g_global->docLink("lost-connection.html", "doc/lost-connection.html");

        parts.append(TR_ERROR3.arg(conn->name()) + ':');
        parts.append(conn->failureMessage());
        parts.append(TR_TEXT3);
        parts.append(TR_TEXT2.arg(link));

        errBox(TR_TITLE2, parts.join(A("<br>")), m_manager->parentWidget())->show();
    }

    // Note: user must manually remove connection
    recheckLocalServer();
    recheckConnected();
}

void
TermListener::reportProxyLost(TermInstance *term, QString reason)
{
    if (m_manager) {
        QStringList parts;
        QString link = g_global->docLink("lost-connection.html", "doc/lost-connection.html");

        parts.append(TR_ERROR3.arg(term->peer()->longname()) + ':');
        parts.append(reason);
        parts.append(TR_TEXT3);
        parts.append(TR_TEXT2.arg(link));

        errBox(TR_TITLE2, parts.join(A("<br>")), m_manager->parentWidget())->show();
    }
}

void
TermListener::destroyConnection(ServerConnection *conn)
{
    conn->stop();

    if (m_transient == conn)
        m_transient = nullptr;
    else if (m_persistent == conn)
        m_persistent = nullptr;

    recheckLocalServer();

    for (auto term: conn->terms()) {
        removeTerm(term);
        term->deleteLater();
    }
    for (auto server: conn->servers()) {
        removeServer(server);
        server->deleteLater(); // Note: this includes conn itself
    }

    recheckConnected();
}

void
TermListener::destroyServer(ServerInstance *server)
{
    for (auto term: server->terms()) {
        removeTerm(term);
        term->deleteLater();
    }

    removeServer(server);
    server->deleteLater();
}

void
TermListener::handleAccept(int fd)
{
    int connfd;

    try {
        connfd = osAccept(fd, nullptr, nullptr);
    }
    catch (const std::exception &e) {
        return;
    }

    if (connfd >= 0)
        try {
            osLocalCredsCheck(connfd);
            (new ReaderConnection)->start(connfd);
        }
        catch (const std::exception &e) {
            close(connfd);
        }
}

void
TermListener::setInputLeader(TermInstance *term)
{
    if (m_inputLeader) {
        m_inputLeader->setInputLeader(false);
        m_inputLeader->setInputFollower(true);
        m_inputFollowers.insert(m_inputLeader);

        term->setInputFollower(false);
        m_inputFollowers.remove(term);
    }

    m_inputLeader = term;
    term->setInputLeader(true);
}

void
TermListener::unsetInputLeader()
{
    if (m_inputLeader) {
        m_inputLeader->setInputLeader(false);
        m_inputLeader = nullptr;
    }

    for (auto t: qAsConst(m_inputFollowers))
        t->setInputFollower(false);

    m_inputFollowers.clear();
}

void
TermListener::setInputFollower(TermInstance *term)
{
    if (m_inputLeader && m_inputLeader != term) {
        term->setInputFollower(true);
        m_inputFollowers.insert(term);
    }
}

void
TermListener::unsetInputFollower(TermInstance *term)
{
    if (m_inputLeader && m_inputLeader != term) {
        term->setInputFollower(false);
        m_inputFollowers.remove(term);
    }
}

bool
TermListener::registerPluginPrompt(QDialog *box)
{
    if (s_pluginPrompt) {
        box->deleteLater();
        return false;
    }

    connect(box, &QObject::destroyed, this, []{ s_pluginPrompt = false; });
    box->show();
    return s_pluginPrompt = true;
}
