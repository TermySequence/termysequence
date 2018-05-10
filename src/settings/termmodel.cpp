// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/color.h"
#include "app/icons.h"
#include "app/selhelper.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/mainwindow.h"
#include "base/server.h"
#include "base/term.h"
#include "base/infoanim.h"
#include "termmodel.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QMenu>
#include <cassert>

#define TR_TEXT1 TL("window-text", "HIDDEN")
#define TR_TEXT2 TL("window-text", "LINK")
#define TR_TEXT3 TL("window-text", "OWNER:%1@%2")
#define TR_TEXT4 TL("window-text", "FOLLOWER")
#define TR_TEXT5 TL("window-text", "LEADER")
#define TR_TEXT6 TL("window-text", "URGENT")
#define TR_TEXT7 TL("window-text", "ALERT")

//
// State tracker
//
struct TermModel::Node {
    NodeLevel m_type;
    int row;
    TermModel *m_model;
    Node *parent;
    QVector<Node*> children;

    ServerInstance *server;
    TermInstance *term;

    QString m_text;
    QIcon m_icon;
    QString m_iconSpec;
    QVariant m_bg, m_fg;

    InfoAnimation *m_animation;

    Node* findServer(ServerInstance *server) const;
    void updateServerDisplay();
    void addServer(ServerInstance *server);
    void updateServer(ServerInstance *server);
    void updateServers();
    void removeServer(ServerInstance *server);
    void sortServers();

    Node* findTerm(TermInstance *term) const;
    std::pair<bool,bool> updateTermDisplay();
    void addTerm(TermInstance *term);
    void flashTerm(TermInstance *term);
    void updateTerm(TermInstance *term, bool animate = true);
    void updateTerms();
    void removeTerm(TermInstance *term);
    void sortTerms();

    QVariant data(int role) const;
    QModelIndex index() const;

    Node(NodeLevel type, TermModel *model, Node *parent);
    ~Node();

    inline const QWidget* parentWidget() const {
        return static_cast<QWidget*>(m_model->QObject::parent());
    }
};

TermModel::Node::Node(NodeLevel type, TermModel *model, Node *p) :
    m_type(type), m_model(model), parent(p)
{
    server = nullptr;
    term = nullptr;

    m_animation = new InfoAnimation(model, (intptr_t)this);
    model->connect(m_animation, SIGNAL(animationSignal(intptr_t)),
                   SLOT(handleNodeAnimation(intptr_t)));
}

TermModel::Node::~Node()
{
    forDeleteAll(children);
    delete m_animation;
}

QVariant
TermModel::Node::data(int role) const
{
    QString tmp;

    switch (m_type) {
    case LevelServer:
    case LevelTerm:
        switch (role) {
        case Qt::DisplayRole:
            return m_text;
        case Qt::ForegroundRole:
            return m_fg;
        case Qt::BackgroundRole:
            return m_animation->blendedVariant(m_bg);
        case Qt::DecorationRole:
            return m_icon;
        case TERM_ROLE_SERVER:
            return QVariant::fromValue((QObject*)server);
        case TERM_ROLE_TERM:
            return QVariant::fromValue((QObject*)term);
        case TERM_ROLE_TYPE:
            return m_type;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QModelIndex
TermModel::Node::index() const
{
    if (parent)
        return m_model->index(row, 0, parent->index());
    else
        return QModelIndex();
}

TermModel::Node *
TermModel::Node::findTerm(TermInstance *term) const
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->term == term)
            return children[row];

    return nullptr;
}

std::pair<bool,bool>
TermModel::Node::updateTermDisplay()
{
    QStringList notes;
    const auto &attr = term->attributes();

    if (m_model->m_manager && m_model->m_manager->isHidden(term))
        notes += m_model->m_text1;

    if (term->peer())
        notes += m_model->m_text2;
    else if (!term->ours()) {
        QString u = attr.value(g_attr_OWNER_USER, g_str_unknown);
        QString h = attr.value(g_attr_OWNER_HOST, g_str_unknown);
        notes += m_model->m_text3.arg(u, h);
    }

    if (term->inputFollower())
        notes += m_model->m_text4;
    else if (term->inputLeader())
        notes += m_model->m_text5;

    if (term->alertUrgent())
        notes += m_model->m_text6;
    else if (term->alert())
        notes += m_model->m_text7;

    QString text = term->title();
    if (!notes.isEmpty())
        text.prepend(L("[%1] ").arg(notes.join(' ')));
    QString stack = attr.value(g_attr_TSQT_INDEX);
    if (!stack.isEmpty())
        text.prepend(stack + A(": "));

    QString iconSpec = term->iconSpec();
    QVariant bg, fg;
    if (m_model->m_colors) {
        bg = QColor(term->bg());
        fg = QColor(term->fg());
    }

    bool textChange = m_text != text || m_iconSpec != iconSpec;
    bool colorChange = m_bg != bg || m_fg != fg;

    if (textChange) {
        m_text = text;
        m_icon = term->icon();
        m_iconSpec = iconSpec;
    }
    if (colorChange) {
        m_bg = bg;
        m_fg = fg;
    }
    return std::make_pair(textChange || colorChange, textChange);
}

void
TermModel::Node::addTerm(TermInstance *term)
{
    // Find the correct insertion point
    const auto &order = m_model->m_manager->order();
    int orow = 0;
    while (order[orow] != term->server())
        ++orow;
    ++orow;
    int row = 0;
    while (row < children.size() && order[orow] != term) {
        ++row;
        ++orow;
    }

    Node *node = new Node(LevelTerm, m_model, this);
    node->row = row;
    node->server = term->server();
    node->term = term;
    node->updateTermDisplay();

    m_model->beginInsertRows(index(), row, row);
    children.insert(row, node);
    m_model->endInsertRows();
}

void
TermModel::Node::flashTerm(TermInstance *term)
{
    for (int row = 0; row < children.size(); ++row) {
        Node *node = children[row];
        if (node->term == term) {
            QModelIndex me = index();
            QModelIndex index = m_model->index(row, 0, me);
            emit m_model->dataChanged(index, index);

            node->m_animation->startColorName(BellFg);
            break;
        }
    }
}

void
TermModel::Node::updateTerm(TermInstance *term, bool animate)
{
    for (int row = 0; row < children.size(); ++row) {
        Node *node = children[row];
        if (node->term == term) {
            auto pair = node->updateTermDisplay();
            if (pair.first) {
                QModelIndex me = index();
                QModelIndex index = m_model->index(row, 0, me);
                emit m_model->dataChanged(index, index);

                if (animate && pair.second)
                    node->m_animation->startVariant(node->m_fg, parentWidget());
            }
            break;
        }
    }
}

void
TermModel::Node::updateTerms()
{
    for (int row = 0; row < children.size(); ++row) {
        Node *node = children[row];
        auto pair = node->updateTermDisplay();
        if (pair.first) {
            QModelIndex me = index();
            QModelIndex index = m_model->index(row, 0, me);
            emit m_model->dataChanged(index, index);

            if (m_model->m_loaded && pair.second)
                node->m_animation->startVariant(node->m_fg, parentWidget());
        }
    }
}

void
TermModel::Node::removeTerm(TermInstance *term)
{
    int row = 0;

    for (; row < children.size(); ++row)
        if (children[row]->term == term)
            break;

    m_model->beginRemoveRows(index(), row, row);
    delete children[row];
    children.removeAt(row);
    for (; row < children.size(); ++row)
        children[row]->row = row;
    m_model->endRemoveRows();
}

TermModel::Node *
TermModel::Node::findServer(ServerInstance *server) const
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->server == server)
            return children[row];

    assert(false);
    return nullptr;
}

inline void
TermModel::Node::updateServerDisplay()
{
    m_text = server->fullname();
    m_icon = server->icon();
}

void
TermModel::Node::addServer(ServerInstance *server)
{
    Node *node = new Node(LevelServer, m_model, this);
    int row = children.size();
    node->row = row;
    node->server = server;
    node->term = nullptr;
    node->updateServerDisplay();

    m_model->beginInsertRows(QModelIndex(), row, row);
    children.push_back(node);
    m_model->endInsertRows();

    if (m_model->m_loaded)
        node->m_animation->startColor(parentWidget());
}

void
TermModel::Node::updateServer(ServerInstance *server)
{
    for (int row = 0; row < children.size(); ++row)
        if (children[row]->server == server) {
            children[row]->updateServerDisplay();

            QModelIndex me = index();
            QModelIndex index = m_model->index(row, 0, me);
            emit m_model->dataChanged(index, index);

            children[row]->m_animation->startColor(parentWidget());
            break;
        }
}

void
TermModel::Node::removeServer(ServerInstance *server)
{
    int row = 0;

    for (; row < children.size(); ++row)
        if (children[row]->server == server)
            break;

    m_model->beginRemoveRows(QModelIndex(), row, row);
    delete children[row];
    children.removeAt(row);
    for (; row < children.size(); ++row)
        children[row]->row = row;
    m_model->endRemoveRows();
}

void
TermModel::Node::sortTerms()
{
    QModelIndex me = index();
    const auto &order = m_model->m_manager->order();
    int orow = 0;
    while (order[orow] != server)
        ++orow;
    ++orow;

    for (int row = 0; row < children.size() - 1; ++row, ++orow) {
        auto target = order[orow];
        if (children[row]->term != target) {
            // Move correct node forward
            int pos = row + 1;
            while (children[pos]->term != target)
                ++pos;

            m_model->beginMoveRows(me, pos, pos, me, row);
            children.move(pos, row);
            for (int i = row; i < children.size(); ++i)
                children[i]->row = i;
            m_model->endMoveRows();
        }
    }
}

void
TermModel::Node::sortServers()
{
    // Total sort
    QModelIndex me = index();
    const auto &servers = m_model->m_manager->servers();
    assert(servers.size() == children.size());

    for (int row = 0; row < children.size() - 1; ++row) {
        auto target = servers[row];
        if (children[row]->server != target) {
            // Move correct node forward
            int pos = row + 1;
            while (children[pos]->server != target)
                ++pos;

            m_model->beginMoveRows(me, pos, pos, me, row);
            children.move(pos, row);
            for (int i = row; i < children.size(); ++i)
                children[i]->row = i;
            m_model->endMoveRows();
        }
    }

    for (auto child: children)
        child->sortTerms();
}

inline void
TermModel::Node::updateServers()
{
    // Total update
    for (int row = 0; row < children.size(); ++row)
        children[row]->updateTerms();
}

//
// Model
//
TermModel::TermModel(TermManager *manager, QWidget *parent) :
    QAbstractItemModel(parent)
{
    m_text1 = TR_TEXT1;
    m_text2 = TR_TEXT2;
    m_text3 = TR_TEXT3;
    m_text4 = TR_TEXT4;
    m_text5 = TR_TEXT5;
    m_text6 = TR_TEXT6;
    m_text7 = TR_TEXT7;

    m_root = new Node(LevelRoot, this, nullptr);
    setManager(manager);
}

TermModel::~TermModel()
{
    delete m_root;
}

void
TermModel::setColors(bool colors)
{
    m_loaded = false;
    m_colors = colors;
    m_root->updateServers();
    m_loaded = true;
}

void
TermModel::setManager(TermManager *manager)
{
    bool haveManager = m_manager;

    if (m_manager) {
        m_manager->disconnect(this);
    }

    if ((m_manager = manager)) {
        m_loaded = false;
        if (!haveManager) {
            for (auto server: manager->servers())
                handleServerAdded(server);
            for (auto term: manager->terms())
                handleTermAdded(term);
        }

        connect(manager, SIGNAL(serverAdded(ServerInstance*)),
                SLOT(handleServerAdded(ServerInstance*)));
        connect(manager, SIGNAL(serverRemoved(ServerInstance*)),
                SLOT(handleServerRemoved(ServerInstance*)));
        connect(manager, SIGNAL(serverHidden(ServerInstance*,size_t,size_t)),
                SLOT(handleServerHidden(ServerInstance*)));

        connect(manager, SIGNAL(termAdded(TermInstance*)),
                SLOT(handleTermAdded(TermInstance*)));
        connect(manager, SIGNAL(termRemoved(TermInstance*,TermInstance*)),
                SLOT(handleTermRemoved(TermInstance*)));
        connect(manager, SIGNAL(termReordered()),
                SLOT(handleTermReordered()));

        m_root->sortServers();
        m_root->updateServers();
        m_loaded = true;
    }
}

void
TermModel::handleServerAdded(ServerInstance *server)
{
    m_root->addServer(server);

    connect(server, &ServerInstance::fullnameChanged, this, &TermModel::handleServerChanged);
    connect(server, &ServerInstance::iconsChanged, this, &TermModel::handleServerChanged);
}

void
TermModel::handleServerRemoved(ServerInstance *server)
{
    server->disconnect(this);
    m_root->removeServer(server);
}

void
TermModel::handleServerHidden(ServerInstance *server)
{
    m_root->findServer(server)->updateTerms();
}

void
TermModel::handleServerChanged()
{
    m_root->updateServer(static_cast<ServerInstance*>(sender()));
}

void
TermModel::handleTermReordered()
{
    m_root->sortServers();
}

void
TermModel::handleTermAdded(TermInstance *term)
{
    m_root->findServer(term->server())->addTerm(term);

    connect(term, &TermInstance::attributeChanged, this, &TermModel::handleTermAttribute);
    connect(term, &TermInstance::iconsChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::colorsChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::alertChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::inputChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::ownershipChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::peerChanged, this, &TermModel::handleTermChanged);
    connect(term, &TermInstance::bellRang, this, &TermModel::handleTermBell);
}

void
TermModel::handleTermRemoved(TermInstance *term)
{
    term->disconnect(this);
    m_root->findServer(term->server())->removeTerm(term);
}

void
TermModel::handleTermChanged()
{
    auto *term = static_cast<TermInstance*>(sender());
    m_root->findServer(term->server())->updateTerm(term);
}

void
TermModel::handleTermBell()
{
    auto *term = static_cast<TermInstance*>(sender());
    m_root->findServer(term->server())->flashTerm(term);
}

void
TermModel::handleTermAttribute(const QString &key)
{
    auto *term = static_cast<TermInstance*>(sender());

    if (key == g_attr_SESSION_TITLE) {
        m_root->findServer(term->server())->updateTerm(term);
    } else if (key == g_attr_TSQT_INDEX) {
        m_root->findServer(term->server())->updateTerm(term, false);
    }
}

void
TermModel::handleNodeAnimation(intptr_t data)
{
    Node *node = reinterpret_cast<Node*>(data);

    QModelIndex index = node->index();
    emit dataChanged(index, index, QVector<int>(1, Qt::BackgroundRole));
}

/*
 * Model functions
 */
int
TermModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

int
TermModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->children.size();
    else if (parent.column() == 0)
        return ((Node*)parent.internalPointer())->children.size();
    else
        return 0;
}

QVariant
TermModel::data(const QModelIndex &index, int role) const
{
    const auto *node = (Node*)index.internalPointer();
    return node ? node->data(role) : QVariant();
}

Qt::ItemFlags
TermModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool
TermModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return !m_root->children.isEmpty();
    if (parent.column())
        return false;

    return !((Node*)parent.internalPointer())->children.isEmpty();
}

QModelIndex
TermModel::parent(const QModelIndex &index) const
{
    auto *state = (Node*)index.internalPointer();
    return state->parent->index();
}

QModelIndex
TermModel::index(int row, int column, const QModelIndex &parent) const
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
TermView::TermView(TermModel *model) :
    m_model(model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    header()->setStretchLastSection(true);
    header()->hide();

    connect(m_model, &TermModel::rowsInserted, this, &TermView::handleRowAdded);
}

void
TermView::handleRowAdded(const QModelIndex &parent)
{
    if (parent.data(TERM_ROLE_TYPE).toInt() == TermModel::LevelServer)
        expand(parent);
}

void
TermView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton) {
        doSelectIndex(this, index, true);
        emit launched();
        event->accept();
    }
}

void
TermView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    MainWindow *window = m_model->manager()->parent();
    QMenu *menu = nullptr;

    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        menu = window->getManagePopup(TERM_SERVERP(index), this);
        break;
    case TermModel::LevelTerm:
        menu = window->getManagePopup(TERM_TERMP(index), this);
        break;
    }

    if (menu)
        menu->popup(event->globalPos());

    event->accept();
}
