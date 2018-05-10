// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QAbstractItemModel>
#include <QTreeView>

class TermManager;
class TermInstance;
class ServerInstance;

#define TERM_ROLE_SERVER        Qt::ItemDataRole(Qt::UserRole)
#define TERM_ROLE_TERM          Qt::ItemDataRole(Qt::UserRole + 1)
#define TERM_ROLE_TYPE          Qt::ItemDataRole(Qt::UserRole + 2)

#define TERM_SERVERP(i) \
    static_cast<ServerInstance*>(i.data(TERM_ROLE_SERVER).value<QObject*>())
#define TERM_TERMP(i) \
    static_cast<TermInstance*>(i.data(TERM_ROLE_TERM).value<QObject*>())

//
// Model
//
class TermModel final: public QAbstractItemModel
{
    Q_OBJECT

private:
    struct Node;
    Node *m_root;

    TermManager *m_manager = nullptr;
    bool m_loaded;
    bool m_colors = false;

    QString m_text1, m_text2, m_text3, m_text4;
    QString m_text5, m_text6, m_text7;

private slots:
    void handleServerAdded(ServerInstance *server);
    void handleServerRemoved(ServerInstance *server);
    void handleServerHidden(ServerInstance *server);
    void handleServerChanged();

    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);
    void handleTermReordered();
    void handleTermChanged();
    void handleTermAttribute(const QString &key);
    void handleTermBell();

    void handleNodeAnimation(intptr_t data);

public:
    enum NodeLevel { LevelRoot, LevelServer, LevelTerm };

    TermModel(TermManager *manager, QWidget *parent);
    ~TermModel();

    inline TermManager* manager() const { return m_manager; }

    void setManager(TermManager *manager);
    void setColors(bool colors);

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
class TermView final: public QTreeView
{
    Q_OBJECT

private:
    TermModel *m_model;

private slots:
    void handleRowAdded(const QModelIndex &parent);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void launched();

public:
    TermView(TermModel *model);

    QModelIndex selectedIndex() const;
};

inline QModelIndex
TermView::selectedIndex() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    return !indexes.isEmpty() ? indexes.at(0) : QModelIndex();
}
