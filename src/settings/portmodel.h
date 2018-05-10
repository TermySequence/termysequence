// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QAbstractItemModel>
#include <QTreeView>

class ServerInstance;
class TermTask;
struct PortFwdRule;

#define PORT_ROLE_SERVER        Qt::ItemDataRole(Qt::UserRole)
#define PORT_ROLE_RULE          Qt::ItemDataRole(Qt::UserRole + 1)
#define PORT_ROLE_TASK          Qt::ItemDataRole(Qt::UserRole + 2)
#define PORT_ROLE_CONN          Qt::ItemDataRole(Qt::UserRole + 3)
#define PORT_ROLE_STARTABLE     Qt::ItemDataRole(Qt::UserRole + 4)
#define PORT_ROLE_CANCELABLE    Qt::ItemDataRole(Qt::UserRole + 5)
#define PORT_ROLE_KILLABLE      Qt::ItemDataRole(Qt::UserRole + 6)
#define PORT_ROLE_TYPE          Qt::ItemDataRole(Qt::UserRole + 7)

#define PORT_SERVERP(i) \
    static_cast<ServerInstance*>(i.data(PORT_ROLE_SERVER).value<QObject*>())
#define PORT_RULEP(i) \
    static_cast<const PortFwdRule*>(i.data(PORT_ROLE_RULE).value<void*>())
#define PORT_TASKP(i) \
    static_cast<PortFwdTask*>(i.data(PORT_ROLE_TASK).value<QObject*>())

//
// Model
//
class PortModel final: public QAbstractItemModel
{
    Q_OBJECT

private:
    struct Node;
    Node *m_root;
    bool m_loaded = false;

private slots:
    void handleServerAdded(ServerInstance *server);
    void handleServerRemoved(ServerInstance *server);
    void handleServerChanged();
    void handlePortsChanged();

    void handleTaskAdded(TermTask *task);
    void handleTaskFinished(TermTask *task);
    void handleTaskRemoved(TermTask *task);

    void handleConnAdded(portfwd_t id);
    void handleConnRemoved(portfwd_t id);
    void handleNodeAnimation(intptr_t data);

public:
    enum NodeLevel { LevelRoot, LevelServer, LevelPort, LevelConn, LevelNoConn };

    PortModel(QWidget *parent);
    ~PortModel();

    inline const Node* root() const { return m_root; }
    bool hasPort(ServerInstance *server, const PortFwdRule *rule);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool hasChildren(const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
};

//
// View
//
class PortView final: public QTreeView
{
    Q_OBJECT

private:
    PortModel *m_model;

private slots:
    void handleRowAdded(const QModelIndex &parent);
    void handleRowChanged(const QModelIndex &topLeft);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void portChanged();
    void launched();

public:
    PortView();

    QModelIndex selectedIndex() const;
    bool hasPort(ServerInstance *server, const PortFwdRule *rule) const;
};

inline QModelIndex
PortView::selectedIndex() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    return !indexes.isEmpty() ? indexes.at(0) : QModelIndex();
}
