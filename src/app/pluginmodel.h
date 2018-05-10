// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QAbstractItemModel>
#include <QTreeView>

class Plugin;
class Feature;

//
// Model
//
class PluginModel final: public QAbstractItemModel
{
    Q_OBJECT

private:
    struct Node;
    Node *m_root;
    bool m_loaded = false;

private slots:
    void handlePluginsReloaded();
    void handlePluginReloaded(Plugin *plugin);
    void handlePluginUnloaded(Plugin *plugin);
    void handleFeatureUnloaded(Feature *feature);
    void handleNodeAnimation(intptr_t data);

public:
    enum NodeLevel { LevelRoot, LevelDisabled, LevelPlugin, LevelFeature };

    PluginModel(QWidget *parent);
    ~PluginModel();

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool hasChildren(const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
};

//
// View
//
class PluginView final: public QTreeView
{
    Q_OBJECT

private:
    PluginModel *m_model;

public:
    PluginView();

    Plugin* selectedPlugin() const;
};
