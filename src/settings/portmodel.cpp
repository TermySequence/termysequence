// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/icons.h"
#include "app/selhelper.h"
#include "base/listener.h"
#include "base/server.h"
#include "base/taskmodel.h"
#include "base/portfwdtask.h"
#include "base/infoanim.h"
#include "portmodel.h"
#include "servinfo.h"
#include "port.h"

#include <QHeaderView>
#include <QMouseEvent>

//
// State tracker
//
struct PortModel::Node {
    NodeLevel m_type;
    int row;
    PortModel *m_model;
    Node *parent;
    QVector<Node*> children;

    QString m_text;
    QIcon m_icon;

    union {
        struct {
            ServerInstance *server;
            ServerSettings *servinfo;
        } s;
        struct {
            PortFwdRule *rule;
            PortFwdTask *task;
        } p;
        struct {
            PortFwdTask::ConnInfo *info;
        } c;
    } m_u;
    union {
        InfoAnimation *anim;
        ProxyAnimation *prox;
    } m_a;

    Node* findServer(ServerInstance *server) const;
    Node* findServer(ServerSettings *servinfo) const;
    void updateServerDisplay();
    void addServer(ServerInstance *server);
    void updateServer(ServerInstance *server);
    void removeServer(ServerInstance *server);

    Node* findPort(PortFwdTask *task) const;
    void updatePortDisplay();
    void updatePorts();
    Node* updatePort(PortFwdTask *task);
    void removePort(PortFwdTask *task);
    void sortPorts(const QModelIndex &me);
    bool hasPort(const PortFwdRule *rule) const;

    void updateConnDisplay();
    void addConns();
    void removeConns();
    void addConn(portfwd_t id);
    void removeConn(portfwd_t id);

    QVariant data(int role) const;
    QModelIndex index() const;

    Node(NodeLevel type, PortModel *model, Node *parent);
    ~Node();

    inline const QWidget* parentWidget() const {
        return static_cast<QWidget*>(m_model->QObject::parent());
    }
};

PortModel::Node::Node(NodeLevel type, PortModel *model, Node *p) :
    m_type(type), m_model(model), parent(p)
{
    memset(&m_u, 0, sizeof(m_u));
    memset(&m_a, 0, sizeof(m_a));

    switch (m_type) {
    case LevelServer:
    case LevelConn:
    case LevelNoConn:
        m_a.anim = new InfoAnimation(model, (intptr_t)this);
        model->connect(m_a.anim, SIGNAL(animationSignal(intptr_t)),
                       SLOT(handleNodeAnimation(intptr_t)));
        break;
    case LevelPort:
        m_a.prox = new ProxyAnimation(model, (intptr_t)this);
        model->connect(m_a.prox, SIGNAL(animationSignal(intptr_t)),
                       SLOT(handleNodeAnimation(intptr_t)));
        break;
    default:
        break;
    }
}

PortModel::Node::~Node()
{
    forDeleteAll(children);

    switch (m_type) {
    case LevelPort:
        delete m_u.p.rule;
        delete m_a.prox;
        break;
    case LevelConn:
        delete m_u.c.info;
        // fallthru
    default:
        delete m_a.anim;
        break;
    }
}

QVariant
PortModel::Node::data(int role) const
{
    QString tmp;

    switch (m_type) {
    case LevelServer:
        switch (role) {
        case Qt::DisplayRole:
            return m_text;
        case Qt::BackgroundRole:
            return m_a.anim->colorVariant();
        case Qt::DecorationRole:
            return m_icon;
        case PORT_ROLE_SERVER:
            return QVariant::fromValue((QObject*)m_u.s.server);
        case PORT_ROLE_RULE:
        case PORT_ROLE_TASK:
            return 0;
        case PORT_ROLE_CONN:
            return INVALID_PORTFWD;
        case PORT_ROLE_STARTABLE:
        case PORT_ROLE_CANCELABLE:
        case PORT_ROLE_KILLABLE:
            return false;
        case PORT_ROLE_TYPE:
            return m_type;
        }
        break;
    case LevelPort:
        switch (role) {
        case Qt::DisplayRole:
            return m_text;
        case Qt::BackgroundRole:
            return m_a.prox->colorVariant();
        case Qt::DecorationRole:
            return m_icon;
        case PORT_ROLE_SERVER:
            return QVariant::fromValue((QObject*)parent->m_u.s.server);
        case PORT_ROLE_RULE:
            return QVariant::fromValue((void*)m_u.p.rule);
        case PORT_ROLE_TASK:
            return QVariant::fromValue((QObject*)m_u.p.task);
        case PORT_ROLE_CONN:
            return INVALID_PORTFWD;
        case PORT_ROLE_STARTABLE:
            return !m_u.p.task || m_u.p.task->finished();
        case PORT_ROLE_CANCELABLE:
            return m_u.p.task && !m_u.p.task->finished();
        case PORT_ROLE_KILLABLE:
            return false;
        case PORT_ROLE_TYPE:
            return m_type;
        }
        break;
    case LevelConn:
        switch (role) {
        case Qt::DisplayRole:
            return m_text;
        case Qt::BackgroundRole:
            return m_a.anim->colorVariant();
        case Qt::DecorationRole:
            return m_icon;
        case PORT_ROLE_SERVER:
            return QVariant::fromValue((QObject*)parent->parent->m_u.s.server);
        case PORT_ROLE_RULE:
            return QVariant::fromValue((void*)parent->m_u.p.rule);
        case PORT_ROLE_TASK:
            return QVariant::fromValue((QObject*)parent->m_u.p.task);
        case PORT_ROLE_CONN:
            return m_u.c.info->id;
        case PORT_ROLE_STARTABLE:
            return false;
        case PORT_ROLE_CANCELABLE:
            return true;
        case PORT_ROLE_KILLABLE:
            return true;
        case PORT_ROLE_TYPE:
            return m_type;
        }
        break;
    case LevelNoConn:
        switch (role) {
        case Qt::DisplayRole:
            return m_text;
        case Qt::BackgroundRole:
            return m_a.anim->colorVariant();
        case Qt::DecorationRole:
            return m_icon;
        case PORT_ROLE_SERVER:
            return QVariant::fromValue((QObject*)parent->parent->m_u.s.server);
        case PORT_ROLE_RULE:
            return QVariant::fromValue((void*)parent->m_u.p.rule);
        case PORT_ROLE_TASK:
            return QVariant::fromValue((QObject*)parent->m_u.p.task);
        case PORT_ROLE_CONN:
            return INVALID_PORTFWD;
        case PORT_ROLE_STARTABLE:
            return false;
        case PORT_ROLE_CANCELABLE:
            return true;
        case PORT_ROLE_KILLABLE:
            return false;
        case PORT_ROLE_TYPE:
            return m_type;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QModelIndex
PortModel::Node::index() const
{
    if (parent)
        return m_model->index(row, 0, parent->index());
    else
        return QModelIndex();
}

void
PortModel::Node::updateConnDisplay()
{
    if (m_type == LevelNoConn) {
        m_text = tr("No active connections");
        m_icon = QIcon();
    } else {
        m_text = L("%1 %2:%3").arg(
            tr("Connection from"), m_u.c.info->caddr, m_u.c.info->cport);
        m_icon = ICON_CONNTYPE_ACTIVE;
    }
}

void
PortModel::Node::addConns()
{
    QModelIndex me = index();

    auto conns = m_u.p.task->connectionsInfo();
    if (conns.isEmpty()) {
        Node *node = new Node(LevelNoConn, m_model, this);
        node->row = 0;
        node->updateConnDisplay();

        m_model->beginInsertRows(me, 0, 0);
        children.push_back(node);
        m_model->endInsertRows();
    } else {
        m_model->beginInsertRows(me, 0, conns.size() - 1);

        for (int row = 0; row < conns.size(); ++row) {
            Node *node = new Node(LevelConn, m_model, this);
            node->row = row;
            node->m_u.c.info = new PortFwdTask::ConnInfo(conns.at(row));
            node->updateConnDisplay();
            children.push_back(node);
        }

        m_model->endInsertRows();
    }
}

void
PortModel::Node::addConn(portfwd_t id)
{
    auto conninfo = m_u.p.task->connectionInfo(id);
    QModelIndex me = index();
    Node *node;

    if (children.size() == 1 && children.front()->m_type == LevelNoConn) {
        node = children.front();
        node->m_type = LevelConn;
        node->m_u.c.info = new PortFwdTask::ConnInfo(conninfo);
        node->updateConnDisplay();

        QModelIndex index = m_model->index(0, 0, me);
        emit m_model->dataChanged(index, index);
    } else {
        node = new Node(LevelConn, m_model, this);
        int row = children.size();
        node->row = row;
        node->m_u.c.info = new PortFwdTask::ConnInfo(conninfo);
        node->updateConnDisplay();

        m_model->beginInsertRows(me, row, row);
        children.push_back(node);
        m_model->endInsertRows();
    }

    if (m_model->m_loaded)
        node->m_a.anim->startColor(parentWidget());
}

void
PortModel::Node::removeConns()
{
    if (!children.isEmpty()) {
        m_model->beginRemoveRows(index(), 0, children.size() - 1);
        forDeleteAll(children);
        children.clear();
        m_model->endRemoveRows();
    }
}

void
PortModel::Node::removeConn(portfwd_t id)
{
    QModelIndex me = index();

    if (children.size() == 1) {
        Node *node = children.front();
        node->m_type = LevelNoConn;
        delete node->m_u.c.info;
        node->updateConnDisplay();

        QModelIndex index = m_model->index(0, 0, me);
        emit m_model->dataChanged(index, index);

        node->m_a.anim->startColor(parentWidget());
    } else {
        int row = 0;

        for (; row < children.size(); ++row)
            if (children[row]->m_u.c.info->id == id)
                break;

        m_model->beginRemoveRows(me, row, row);
        delete children[row];
        children.removeAt(row);
        for (; row < children.size(); ++row)
            children[row]->row = row;
        m_model->endRemoveRows();
    }
}

PortModel::Node *
PortModel::Node::findPort(PortFwdTask *task) const
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->m_u.p.task == task)
            return children[row];

    return nullptr;
}

void
PortModel::Node::updatePortDisplay()
{
    if (m_u.p.task && !m_u.p.task->finished()) {
        m_icon = m_u.p.task->typeIcon();
    } else {
        m_icon = QIcon();
    }

    m_text = L("(%1) %2 \u2192 (%3) %4");
    if (m_u.p.rule->islocal) {
        m_text = m_text.arg(tr("Local"), m_u.p.rule->localStr(),
                            tr("Remote"), m_u.p.rule->remoteStr());
    } else {
        m_text = m_text.arg(tr("Remote"), m_u.p.rule->remoteStr(),
                            tr("Local"), m_u.p.rule->localStr());
    }
    if (m_u.p.task) {
        m_text += A(" - ");
        m_text += m_u.p.task->statusStr();
    }
}

void
PortModel::Node::updatePorts()
{
    PortFwdList rules(m_u.s.servinfo->ports());
    QModelIndex me = index();

    for (int row = 0; row < children.size(); ++row) {
        auto node = children[row];
        if (rules.contains(*node->m_u.p.rule)) {
            rules.removeOne(*node->m_u.p.rule);
        }
        else if (!node->m_u.p.task) {
            m_model->beginRemoveRows(me, row, row);
            delete node;
            children.removeAt(row);
            for (int i = row; i < children.size(); ++i)
                children[i]->row = i;
            m_model->endRemoveRows();
            --row;
        }
    }

    for (auto &i: qAsConst(rules)) {
        Node *node = new Node(LevelPort, m_model, this);
        int row = children.size();
        node->row = row;
        node->m_u.p.rule = new PortFwdRule(i);
        node->updatePortDisplay();

        m_model->beginInsertRows(me, row, row);
        children.push_back(node);
        m_model->endInsertRows();
    }
}

bool
PortModel::Node::hasPort(const PortFwdRule *rule) const
{
    for (int row = 0; row < children.size(); ++row)
        if (*children[row]->m_u.p.rule == *rule)
            return true;

    return false;
}

void
PortModel::Node::sortPorts(const QModelIndex &me)
{
    for (int row = 0, pos = 0; row < children.size(); ++row) {
        auto node = children[row];
        if (node->m_u.p.task && !node->m_u.p.task->finished()) {
            if (row > pos) {
                // Swap rows
                m_model->beginMoveRows(me, row, row, me, pos);
                children.move(row, pos);
                for (int i = pos; i <= row; ++i)
                    children[i]->row = i;
                m_model->endMoveRows();
            }
            ++pos;
        }
    }
}

void
PortModel::Node::removePort(PortFwdTask *task)
{
    PortFwdList rules(m_u.s.servinfo->ports());
    QModelIndex me = index();

    for (int row = 0; row < children.size(); ++row) {
        auto node = children[row];
        if (node->m_u.p.task == task) {
            if (rules.contains(*node->m_u.p.rule)) {
                sortPorts(me);
            } else {
                m_model->beginRemoveRows(me, row, row);
                delete node;
                children.removeAt(row);
                for (int i = row; i < children.size(); ++i)
                    children[i]->row = i;
                m_model->endRemoveRows();
            }
            break;
        }
    }
}

PortModel::Node *
PortModel::Node::updatePort(PortFwdTask *task)
{
    QModelIndex me = index();
    Node *node;

    int row;
    for (row = 0; row < children.size(); ++row) {
        node = children[row];
        if (node->m_u.p.task != task) {
            if (task->config() == *node->m_u.p.rule) {
                // Don't replace a running task
                if (node->m_u.p.task && !node->m_u.p.task->finished())
                    return nullptr;

                node->m_u.p.task = task;
                node->m_a.prox->setSource(task->animation());
            } else {
                continue;
            }
        }

        node->updatePortDisplay();
        QModelIndex index = m_model->index(row, 0, me);
        emit m_model->dataChanged(index, index);
        goto finished;
    }

    node = new Node(LevelPort, m_model, this);
    node->row = row;
    node->m_u.p.rule = new PortFwdRule(task->config());
    node->m_u.p.task = task;
    node->m_a.prox->setSource(task->animation());
    node->updatePortDisplay();

    m_model->beginInsertRows(me, row, row);
    children.push_back(node);
    m_model->endInsertRows();

finished:
    // Sort list
    sortPorts(me);
    return node;
}

PortModel::Node *
PortModel::Node::findServer(ServerInstance *server) const
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->m_u.s.server == server)
            return children[row];

    return nullptr;
}

PortModel::Node *
PortModel::Node::findServer(ServerSettings *servinfo) const
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->m_u.s.servinfo == servinfo)
            return children[row];

    return nullptr;
}

inline void
PortModel::Node::updateServerDisplay()
{
    m_text = m_u.s.servinfo->fullname();
    m_icon = m_u.s.server->icon();
}

void
PortModel::Node::addServer(ServerInstance *server)
{
    Node *node = new Node(LevelServer, m_model, this);
    int row = children.size();
    node->row = row;
    node->m_u.s.server = server;
    node->m_u.s.servinfo = server->serverInfo();
    node->updateServerDisplay();

    m_model->beginInsertRows(QModelIndex(), row, row);
    children.push_back(node);
    m_model->endInsertRows();

    node->updatePorts();

    if (m_model->m_loaded)
        node->m_a.anim->startColor(parentWidget());
}

void
PortModel::Node::updateServer(ServerInstance *server)
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->m_u.s.server == server) {
            children[row]->updateServerDisplay();

            QModelIndex me = index();
            QModelIndex index = m_model->index(row, 0, me);
            emit m_model->dataChanged(index, index);

            children[row]->m_a.anim->startColor(parentWidget());
            break;
        }
}

void
PortModel::Node::removeServer(ServerInstance *server)
{
    int row = 0;

    for (; row < children.size(); ++row)
        if (children[row]->m_u.s.server == server)
            break;

    m_model->beginRemoveRows(QModelIndex(), row, row);
    delete children[row];
    children.removeAt(row);
    for (; row < children.size(); ++row)
        children[row]->row = row;
    m_model->endRemoveRows();
}

//
// Model
//
PortModel::PortModel(QWidget *parent) :
    QAbstractItemModel(parent)
{
    connect(g_listener, SIGNAL(serverAdded(ServerInstance*)),
            SLOT(handleServerAdded(ServerInstance*)));
    connect(g_listener, SIGNAL(serverRemoved(ServerInstance*)),
            SLOT(handleServerRemoved(ServerInstance*)));

    connect(g_listener->taskmodel(), SIGNAL(taskAdded(TermTask*)),
            SLOT(handleTaskAdded(TermTask*)));
    connect(g_listener->taskmodel(), SIGNAL(taskFinished(TermTask*)),
            SLOT(handleTaskFinished(TermTask*)));
    connect(g_listener->taskmodel(), SIGNAL(taskRemoved(TermTask*)),
            SLOT(handleTaskRemoved(TermTask*)));

    m_root = new Node(LevelRoot, this, nullptr);

    for (auto server: g_listener->servers())
        handleServerAdded(server);
    for (auto task: g_listener->taskmodel()->tasks())
        handleTaskAdded(task);

    m_loaded = true;
}

PortModel::~PortModel()
{
    delete m_root;
}

void
PortModel::handleServerAdded(ServerInstance *server)
{
    m_root->addServer(server);

    connect(server->serverInfo(), SIGNAL(portsChanged()), SLOT(handlePortsChanged()));
    connect(server, &ServerInstance::fullnameChanged, this, &PortModel::handleServerChanged);
    connect(server, &ServerInstance::iconsChanged, this, &PortModel::handleServerChanged);
}

void
PortModel::handleServerRemoved(ServerInstance *server)
{
    server->serverInfo()->disconnect(this);
    server->disconnect(this);
    m_root->removeServer(server);
}

void
PortModel::handlePortsChanged()
{
    m_root->findServer(static_cast<ServerSettings*>(sender()))->updatePorts();
}

void
PortModel::handleServerChanged()
{
    m_root->updateServer(static_cast<ServerInstance*>(sender()));
}

void
PortModel::handleTaskAdded(TermTask *t)
{
    PortFwdTask *task;
    Node *server, *port;

    if (t->longRunning() && (task = qobject_cast<PortFwdTask*>(t)) &&
        (server = m_root->findServer(task->server())) &&
        (port = server->updatePort(task)) &&
        !task->finished())
    {
        connect(task, SIGNAL(connectionAdded(portfwd_t)),
                SLOT(handleConnAdded(portfwd_t)));
        connect(task, SIGNAL(connectionRemoved(portfwd_t)),
                SLOT(handleConnRemoved(portfwd_t)));
        port->addConns();
    }
}

void
PortModel::handleTaskFinished(TermTask *t)
{
    PortFwdTask *task;
    Node *server, *port;

    if (t->longRunning() && (task = qobject_cast<PortFwdTask*>(t))) {
        task->disconnect(this);
        if ((server = m_root->findServer(task->server())) &&
            (port = server->updatePort(task)))
        {
            port->removeConns();
        }
    }
}

void
PortModel::handleTaskRemoved(TermTask *t)
{
    PortFwdTask *task;
    Node *server;

    if (t->longRunning() && (task = qobject_cast<PortFwdTask*>(t)) &&
        (server = m_root->findServer(task->server())))
    {
        server->removePort(task);
    }
}

void
PortModel::handleNodeAnimation(intptr_t data)
{
    Node *node = reinterpret_cast<Node*>(data);

    QModelIndex index = node->index();
    emit dataChanged(index, index, QVector<int>(1, Qt::BackgroundRole));
}

void
PortModel::handleConnAdded(portfwd_t id)
{
    PortFwdTask *task = static_cast<PortFwdTask*>(sender());
    Node *port = m_root->findServer(task->server())->findPort(task);
    port->addConn(id);
}

void
PortModel::handleConnRemoved(portfwd_t id)
{
    PortFwdTask *task = static_cast<PortFwdTask*>(sender());
    Node *port = m_root->findServer(task->server())->findPort(task);
    port->removeConn(id);
}

/*
 * Model functions
 */
int
PortModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

int
PortModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->children.size();
    else if (parent.column() == 0)
        return ((Node*)parent.internalPointer())->children.size();
    else
        return 0;
}

QVariant
PortModel::data(const QModelIndex &index, int role) const
{
    const auto *node = (Node*)index.internalPointer();
    return node ? node->data(role) : QVariant();
}

Qt::ItemFlags
PortModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool
PortModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return !m_root->children.isEmpty();
    if (parent.column())
        return false;

    return !((Node*)parent.internalPointer())->children.isEmpty();
}

QModelIndex
PortModel::parent(const QModelIndex &index) const
{
    auto *state = (Node*)index.internalPointer();
    return state->parent->index();
}

QModelIndex
PortModel::index(int row, int column, const QModelIndex &parent) const
{
    Node *state;

    if (parent.isValid())
        state = (Node*)parent.internalPointer();
    else
        state = m_root;

    if (row < state->children.size())
        return createIndex(row, column, (void*)state->children.at(row));
    else
        return QModelIndex();
}

//
// View
//
PortView::PortView()
{
    m_model = new PortModel(this);

    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    header()->setStretchLastSection(true);
    header()->hide();

    connect(m_model, &PortModel::rowsInserted, this, &PortView::handleRowAdded);
    connect(m_model, &PortModel::dataChanged, this, &PortView::handleRowChanged);
}

bool
PortView::hasPort(ServerInstance *server, const PortFwdRule *rule) const
{
    return m_model->root()->findServer(server)->hasPort(rule);
}

void
PortView::handleRowAdded(const QModelIndex &parent)
{
    switch (parent.data(PORT_ROLE_TYPE).toInt()) {
    case PortModel::LevelServer:
    case PortModel::LevelPort:
        expand(parent);
    }
}

void
PortView::handleRowChanged(const QModelIndex &topLeft)
{
    if (topLeft.data(PORT_ROLE_TYPE).toInt() == PortModel::LevelPort)
        emit portChanged();
}

void
PortView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton) {
        doSelectIndex(this, index, true);
        emit launched();
        event->accept();
    }
}
