// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "pluginmodel.h"
#include "attrbase.h"
#include "feature.h"
#include "icons.h"
#include "plugin.h"
#include "base/infoanim.h"
#include "settings/settings.h"

#include <QHeaderView>

//
// State tracker
//
struct PluginModel::Node {
    NodeLevel m_type;
    int row;
    PluginModel *m_model;
    Node *parent;
    QVector<Node*> children;

    union {
        struct {
            const Plugin *plugin;
        } p;
        struct {
            const Feature *feature;
        } m;
    } m_u;
    QString text;

    InfoAnimation *m_animation;

    void clear();
    void addDisabled();

    Node* findPlugin(const Plugin *plugin);
    void addPlugin(const Plugin *plugin);
    void updatePlugin(const Plugin *plugin);
    void removePlugin(const Plugin *plugin);

    void updateFeatures();
    void removeFeature(const Feature *feature);

    QVariant data(int role);
    QModelIndex index() const;

    Node(NodeLevel type, PluginModel *model, Node *parent);
    ~Node();

    inline const QWidget* parentWidget() const {
        return static_cast<QWidget*>(m_model->QObject::parent());
    }
};

PluginModel::Node::Node(NodeLevel type, PluginModel *model, Node *p) :
    m_type(type), m_model(model), parent(p)
{
    memset(&m_u, 0, sizeof(m_u));

    switch (m_type) {
    case LevelPlugin:
    case LevelFeature:
        m_animation = new InfoAnimation(model, (intptr_t)this);
        model->connect(m_animation, SIGNAL(animationSignal(intptr_t)),
                       SLOT(handleNodeAnimation(intptr_t)));
        break;
    default:
        m_animation = nullptr;
        break;
    }
}

PluginModel::Node::~Node()
{
    forDeleteAll(children);
    delete m_animation;
}

QVariant
PluginModel::Node::data(int role)
{
    QString tmp;

    switch (m_type) {
    case LevelDisabled:
        switch (role) {
        case Qt::DisplayRole:
            return text;
        case Qt::DecorationRole:
            return ICON_PLUGIN_DISABLED;
        }
        break;
    case LevelPlugin:
        switch (role) {
        case Qt::DisplayRole:
            return text;
        case Qt::BackgroundRole:
            return m_animation->colorVariant();
        case Qt::UserRole:
            return QVariant::fromValue((QObject*)m_u.p.plugin);
        }
        break;
    case LevelFeature:
        switch (role) {
        case Qt::DisplayRole:
            return text;
        case Qt::DecorationRole:
            switch (m_u.m.feature->type()) {
            case Feature::ActionFeatureType:
                return ICON_PLUGIN_ACTION;
            case Feature::TipFeatureType:
                return ICON_PLUGIN_TOTD;
            default:
                return ICON_PLUGIN_PARSER;
            }
        case Qt::BackgroundRole:
            return m_animation->colorVariant();
        case Qt::UserRole:
            return QVariant::fromValue((QObject*)parent->m_u.p.plugin);
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QModelIndex
PluginModel::Node::index() const
{
    if (parent)
        return m_model->index(row, 0, parent->index());
    else
        return QModelIndex();
}

void
PluginModel::Node::updateFeatures()
{
    clear();

    for (auto feature: m_u.p.plugin->features()) {
        Node *node = new Node(LevelFeature, m_model, this);
        int row = children.size();
        node->row = row;
        node->m_u.m.feature = feature;
        node->text = L("%1 (%2)").arg(feature->name(), feature->typeString());

        m_model->beginInsertRows(index(), row, row);
        children.push_back(node);
        m_model->endInsertRows();

        if (m_model->m_loaded)
            node->m_animation->startColor(parentWidget());
    }
}

void
PluginModel::Node::removeFeature(const Feature *feature)
{
    QModelIndex me = index();

    for (int row = 0; row < children.size(); ++row) {
        auto node = children[row];
        if (node->m_u.m.feature == feature) {
            m_model->beginRemoveRows(me, row, row);
            delete node;
            children.removeAt(row);
            for (int i = row; i < children.size(); ++i)
                children[i]->row = i;
            m_model->endRemoveRows();
            break;
        }
    }
}

PluginModel::Node*
PluginModel::Node::findPlugin(const Plugin *plugin)
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->m_u.p.plugin == plugin)
            return children[row];

    return nullptr;
}

void
PluginModel::Node::addPlugin(const Plugin *plugin)
{
    Node *node = new Node(LevelPlugin, m_model, this);
    int row = children.size();
    node->row = row;
    node->m_u.p.plugin = plugin;
    node->text = L("%1 %2 - %3").arg(plugin->name(), plugin->version(),
                                     plugin->description());

    m_model->beginInsertRows(QModelIndex(), row, row);
    children.push_back(node);
    m_model->endInsertRows();

    node->updateFeatures();

    if (m_model->m_loaded)
        node->m_animation->startColor(parentWidget());
}

void
PluginModel::Node::removePlugin(const Plugin *plugin)
{
    QModelIndex me = index();

    for (int row = 0; row < children.size(); ++row) {
        auto node = children[row];
        if (node->m_u.p.plugin == plugin) {
            m_model->beginRemoveRows(me, row, row);
            delete node;
            children.removeAt(row);
            for (int i = row; i < children.size(); ++i)
                children[i]->row = i;
            m_model->endRemoveRows();
            break;
        }
    }
}

void
PluginModel::Node::updatePlugin(const Plugin *plugin)
{
    QModelIndex me = index();

    int row;
    for (row = 0; row < children.size(); ++row) {
        Node *node = children[row];
        if (node->m_u.p.plugin == plugin) {
            node->updateFeatures();
            node->text = L("%1 %2 - %3").arg(plugin->name(), plugin->version(),
                                             plugin->description());
            QModelIndex index = m_model->index(row, 0, me);
            emit m_model->dataChanged(index, index);

            node->m_animation->startColor(parentWidget());
            break;
        }
    }
}

void
PluginModel::Node::addDisabled()
{
    Node *node = new Node(LevelDisabled, m_model, this);
    node->row = 0;
    node->text = A("Plugins disabled: ");

    QVariant v = qApp->property(OBJPROP_V8_ERROR);
    node->text += !v.isNull() ?
        v.toString() :
        A("--noplugins option set");

    m_model->beginInsertRows(QModelIndex(), 0, 0);
    children.push_back(node);
    m_model->endInsertRows();
}

void
PluginModel::Node::clear()
{
    if (!children.isEmpty()) {
        m_model->beginRemoveRows(index(), 0, children.size() - 1);
        forDeleteAll(children);
        children.clear();
        m_model->endRemoveRows();
    }
}

//
// Model
//
PluginModel::PluginModel(QWidget *parent) :
    QAbstractItemModel(parent)
{
    connect(g_settings, SIGNAL(pluginsReloaded()), SLOT(handlePluginsReloaded()));
    connect(g_settings, SIGNAL(pluginReloaded(Plugin*)),
            SLOT(handlePluginReloaded(Plugin*)));
    connect(g_settings, SIGNAL(pluginUnloaded(Plugin*)),
            SLOT(handlePluginUnloaded(Plugin*)));

    m_root = new Node(LevelRoot, this, nullptr);

    handlePluginsReloaded();
    m_loaded = true;
}

PluginModel::~PluginModel()
{
    delete m_root;
}

void
PluginModel::handlePluginsReloaded()
{
    m_root->clear();

    if (i) {
        for (auto plugin: g_settings->plugins()) {
            m_root->addPlugin(plugin);
            connect(plugin, SIGNAL(featureUnloaded(Feature*)),
                    SLOT(handleFeatureUnloaded(Feature*)));
        }
    } else {
        m_root->addDisabled();
    }
}

void
PluginModel::handlePluginReloaded(Plugin *plugin)
{
    m_root->updatePlugin(plugin);
}

void
PluginModel::handlePluginUnloaded(Plugin *plugin)
{
    m_root->removePlugin(plugin);
}

void
PluginModel::handleFeatureUnloaded(Feature *feature)
{
    auto *node = m_root->findPlugin(static_cast<Plugin*>(sender()));
    if (node)
        node->removeFeature(feature);
}

void
PluginModel::handleNodeAnimation(intptr_t data)
{
    Node *node = reinterpret_cast<Node*>(data);

    QModelIndex index = node->index();
    emit dataChanged(index, index, QVector<int>(1, Qt::BackgroundRole));
}

/*
 * Model functions
 */
int
PluginModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

int
PluginModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->children.size();
    else if (parent.column() == 0)
        return ((Node*)parent.internalPointer())->children.size();
    else
        return 0;
}

QVariant
PluginModel::data(const QModelIndex &index, int role) const
{
    if (index.internalPointer())
        return ((Node*)index.internalPointer())->data(role);
    else
        return QVariant();
}

Qt::ItemFlags
PluginModel::flags(const QModelIndex &index) const
{
    if (index.internalPointer())
        if (((Node*)index.internalPointer())->m_type == LevelPlugin)
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled;
}

bool
PluginModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return !m_root->children.isEmpty();
    if (parent.column())
        return false;

    return !((Node*)parent.internalPointer())->children.isEmpty();
}

QModelIndex
PluginModel::parent(const QModelIndex &index) const
{
    auto *state = (Node*)index.internalPointer();
    return state->parent->index();
}

QModelIndex
PluginModel::index(int row, int column, const QModelIndex &parent) const
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
PluginView::PluginView()
{
    m_model = new PluginModel(this);

    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    header()->setStretchLastSection(true);
    header()->hide();
}

Plugin *
PluginView::selectedPlugin() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();

    return !indexes.isEmpty() ?
        static_cast<Plugin*>(indexes.at(0).data(Qt::UserRole).value<QObject*>()) :
        nullptr;
}
