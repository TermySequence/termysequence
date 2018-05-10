// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "lib/types.h"
#include "lib/uuid.h"

#include <QObject>
#include <QHash>
#include <QSet>
#include <QSize>
#include <unordered_map>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
class QDialog;
QT_END_NAMESPACE
class TermManager;
class TermInstance;
class ServerInstance;
class ServerConnection;
class TermBlinkTimer;
class TermEffectTimer;
class TermScrollTimer;
class TermFetchTimer;
class JobModel;
class NoteModel;
class TaskModel;
class ProfileSettings;
class ConnectSettings;
class Region;
struct TermState;

class TermListener final: public QObject
{
    Q_OBJECT

private:
    Tsq::Uuid m_id{true};

    QHash<Tsq::Uuid,TermInstance*> m_terms;
    QHash<Tsq::Uuid,ServerInstance*> m_servers;
    QList<ServerInstance*> m_servlist;

    QList<TermManager*> m_managers;
    QList<TermManager*> m_activity;
    TermManager *m_manager = nullptr;

    JobModel *m_jobmodel;
    NoteModel *m_notemodel;
    TaskModel *m_taskmodel;
    TermBlinkTimer *m_blink;
    TermEffectTimer *m_effects;
    TermScrollTimer *m_scroll;
    TermFetchTimer *m_fetch;

    ServerConnection *m_transient = nullptr;
    ServerConnection *m_persistent = nullptr;
    ServerInstance *m_localServer = nullptr;

    TermInstance *m_inputLeader = nullptr;
    QSet<TermInstance*> m_inputFollowers;

    QSocketNotifier *m_notifier;
    std::unordered_map<std::string,std::string> m_attributes;
    QString m_idStr;

    bool m_commandMode;
    bool m_presMode = false;
    bool m_connected = false;
    char m_failureType = 0;
    unsigned m_flags;
    QString m_failureTitle, m_failureMsg;

    QString makeFailureMessage(QStringList &parts);
    void launchStartupTerminals(ServerInstance *server);

    void hookupInitial(ServerConnection *conn);
    void hookup(ServerConnection *conn);
    void recheckLocalServer();
    void recheckConnected();

signals:
    void commandModeChanged(bool commandMode);
    void presModeChanged(bool presMode);
    void flagsChanged(unsigned flags);

    void serverAdded(ServerInstance *server);
    void serverRemoved(ServerInstance *server);

    void termAdded(TermInstance *term);
    void termRemoved(TermInstance *term);

    void managerAdded(TermManager *manager);
    void managerRemoved(TermManager *manager);

    void ready();
    void connectedChanged(bool connected);

private slots:
    void handleInitialReady();
    void handleServerReady();
    void handleConnectionReady();
    void handleConnectionFailed();

    void handleManagerDestroyed(QObject *object);
    void handleStackReordered();

    void handleAccept(int fd);

public:
    TermListener();
    inline const Tsq::Uuid& id() const { return m_id; }
    inline const QString& idStr() const { return m_idStr; }
    inline const auto& attributes() const { return m_attributes; }
    inline bool commandMode() const { return m_commandMode; }
    inline bool presMode() const { return m_presMode; }
    inline unsigned flags() const { return m_flags; }

    void setCommandMode(bool commandMode);
    void setPresMode(bool presMode);
    void start();
    void quit();

    inline TermBlinkTimer* blink() { return m_blink; }
    inline TermEffectTimer* effects() { return m_effects; }
    inline TermScrollTimer* scroll() { return m_scroll; }
    inline JobModel* jobmodel() { return m_jobmodel; }
    inline NoteModel* notemodel() { return m_notemodel; }
    inline TaskModel* taskmodel() { return m_taskmodel; }

    inline bool connected() const { return m_connected; }
    inline char failureType() const { return m_failureType; }
    inline const QString& failureTitle() const { return m_failureTitle; }
    inline const QString& failureMsg() const { return m_failureMsg; }
    inline void clearFailure() { m_failureType = 0; }

    inline const auto& managers() const { return m_managers; }
    inline TermManager* activeManager() { return m_manager; }
    TermManager* createInitialManager();
    void addManager(TermManager *manager);
    void setManager(TermManager *manager);

    void addServer(ServerInstance *server);
    void removeServer(ServerInstance *server);
    void destroyServer(ServerInstance *server);
    void destroyConnection(ServerConnection *conn);
    inline ServerInstance* localServer() { return m_localServer; }
    inline ServerConnection* transientServer() { return m_transient; }
    inline ServerConnection* persistentServer() { return m_persistent; }
    inline ServerInstance* lookupServer(const Tsq::Uuid &id) { return m_servers.value(id); }
    inline auto servers() const { return m_servlist; }

    void addTerm(TermInstance *term);
    void removeTerm(TermInstance *term);
    inline TermInstance* lookupTerm(const Tsq::Uuid &id) { return m_terms.value(id); }
    TermInstance* findTerm(TermManager *target, const TermState *rec) const;
    TermInstance* nextAvailableTerm(TermManager *target) const;

    inline TermInstance* inputLeader() const { return m_inputLeader; }
    void setInputLeader(TermInstance *term);
    void unsetInputLeader();
    void setInputFollower(TermInstance *term);
    void unsetInputFollower(TermInstance *term);

    QObject* launchConnection(ConnectSettings *info, TermManager *manager);
    void reportConnectionLost(ServerConnection *conn);
    void reportProxyLost(TermInstance *term, QString reason);

    bool registerPluginPrompt(QDialog *box);

public:
    void pushServerAttribute(ServerInstance *server, const QString &key, const QString &value);
    void pushServerAttributeRemove(ServerInstance *server, const QString &key);
    void pushServerMonitorInput(ServerInstance *server, const QByteArray &data);
    void pushServerAvatarRequest(ServerInstance *server, const Tsq::Uuid &id);
    void pushTermCreate(TermInstance *term, QSize size);
    void pushTermDuplicate(TermInstance *term, QSize size, const Tsq::Uuid &source);
    void pushTermRemove(TermInstance *term);
    void pushTermDisconnect(TermInstance *term);
    void pushTermInput(TermInstance *term, const QByteArray &data);
    void pushTermMouseEvent(TermInstance *term, uint64_t flags, int x, int y);
    void pushTermResize(TermInstance *term, QSize size);
    void pushTermFetch(TermInstance *term, index_t start, index_t end, uint8_t bufid);
    void pushTermGetImage(TermInstance *term, contentid_t id);
    void pushTermScrollLock(TermInstance *term);
    void pushTermAttribute(TermInstance *term, const QString &key, const QString &value);
    void pushTermAttributes(TermInstance *term, const AttributeMap &map);
    void pushTermAttributeRemove(TermInstance *term, const QString &key);
    void pushTermOwnership(TermInstance *term);
    void pushTermReset(TermInstance *term, unsigned flags);
    void pushTermCaporder(TermInstance *term, uint8_t caporder);
    void pushTermSignal(TermInstance *term, unsigned signal);
    void pushRegionCreate(TermInstance *term, const Region *region);
    void pushRegionRemove(TermInstance *term, regionid_t region);
};

extern TermListener *g_listener;
