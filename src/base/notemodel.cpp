// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "notemodel.h"
#include "jobviewitem.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "dockwidget.h"
#include "mainwindow.h"
#include "infoanim.h"
#include "thumbicon.h"
#include "settings/global.h"
#include "settings/state.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>
#include <QApplication>
#include <QTimer>
#include <algorithm>

#define STATE_VERSION 1
#define MODEL_VERSION 1

//
// Model
//
NoteModel::NoteModel(TermListener *parent) : QAbstractTableModel(parent)
{
    connect(parent, SIGNAL(termAdded(TermInstance*)), SLOT(handleTermAdded(TermInstance*)));
    connect(parent, SIGNAL(termRemoved(TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));

    m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, A("note"));

    connect(g_global, SIGNAL(settingsLoaded()), SLOT(relimit()));
    m_limit = g_global->jobLimit();
}

void
NoteModel::clearAnimations()
{
    for (auto &&i: m_animations) {
        m_notes[i.first].fade = 0;
        i.second->deleteLater();
    }

    m_animations.clear();
}

void
NoteModel::startAnimation(size_t row)
{
    for (auto &&i: m_animations) {
        if (i.first == row) {
            i.second->stop();
            i.second->start();
            return;
        }
    }
    if (m_animations.size() < MAX_NOTE_ANIMATIONS) {
        auto *animation = new InfoAnimation(this, row);
        m_animations.push_back(std::make_pair(row, animation));
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
        connect(animation, SIGNAL(finished()), SLOT(handleAnimationFinished()));
        animation->start();
    }
}

void
NoteModel::handleAnimation(intptr_t row)
{
    auto *animation = static_cast<InfoAnimation*>(sender());
    m_notes[row].fade = animation->fade();

    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, NOTE_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::UserRole));
}

void
NoteModel::handleAnimationFinished()
{
    auto *animation = static_cast<InfoAnimation*>(sender());
    for (auto i = m_animations.begin(), j = m_animations.end(); i != j; ++i)
        if (i->second == animation) {
            i->second->deleteLater();
            m_animations.erase(i);
            break;
        }
}

void
NoteModel::relimit()
{
    size_t limit = g_global->jobLimit();

    if (m_limit != limit) {
        if (m_limit > limit) {
            clearAnimations();
            beginResetModel();
            while (m_notes.size() > limit)
                m_notes.pop_front();
            endResetModel();
        }
        m_limit = limit;
    }
}

void
NoteModel::handleTermAdded(TermInstance *term)
{
    connect(term->buffers(), SIGNAL(noteChanged(TermInstance*,Region*)), SLOT(handleNoteChanged(TermInstance*,Region*)));
    connect(term->buffers(), SIGNAL(noteDeleted(TermInstance*,Region*)), SLOT(handleNoteDeleted(TermInstance*,Region*)));
    connect(term, SIGNAL(paletteChanged()), SLOT(handleTermChanged()));
    connect(term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(handleTermChanged()));
}

void
NoteModel::handleTermChanged()
{
    QObject *term = sender();

    QModelIndex start;
    int row = 0;
    bool flag = false;

    for (auto &note: m_notes)
    {
        if (note.term == term) {
            note.recolor();
            if (!flag) {
                start = createIndex(row, 0);
                flag = true;
            }
        } else if (flag) {
            emit dataChanged(start, createIndex(row, NOTE_N_COLUMNS - 1));
            flag = false;
        }

        ++row;
    }

    if (flag)
        emit dataChanged(start, createIndex(row - 1, NOTE_N_COLUMNS - 1));
}

void
NoteModel::handleTermRemoved(TermInstance *term)
{
    QModelIndex start;
    int row = 0;
    bool flag = false;

    for (auto &note: m_notes)
    {
        if (note.term == term) {
            note.term = nullptr;
            if (!flag) {
                start = createIndex(row, 0);
                flag = true;
            }
        } else if (flag) {
            emit dataChanged(start, createIndex(row, NOTE_N_COLUMNS - 1));
            flag = false;
        }

        ++row;
    }

    if (flag)
        emit dataChanged(start, createIndex(row - 1, NOTE_N_COLUMNS - 1));
}

void
NoteModel::handleNoteDeleted(TermInstance *term, Region *region)
{
    int64_t started = region->attributes[g_attr_REGION_STARTED].toLongLong();
    TermNote note(started);

    auto range = std::equal_range(m_notes.begin(), m_notes.end(), note);
    auto i = range.first, j = range.second;

    while (i != j) {
        if (i->term == term && i->region == region->id()) {
            size_t row = i - m_notes.begin();

            // Remove
            beginRemoveRows(QModelIndex(), row, row);
            m_notes.erase(i);
            endRemoveRows();

            // Adjust animations
            for (auto k = m_animations.begin(); k != m_animations.end(); ) {
                if (k->first == row) {
                    k->second->deleteLater();
                    k = m_animations.erase(k);
                    continue;
                }
                if (k->first > row) {
                    k->second->setData(--k->first);
                }
                ++k;
            }

            break;
        }
        ++i;
    }
}

void
NoteModel::handleNoteChanged(TermInstance *term, Region *region)
{
    int64_t started = region->attributes[g_attr_REGION_STARTED].toLongLong();
    TermNote note(started);

    auto range = std::equal_range(m_notes.begin(), m_notes.end(), note);
    auto i = range.first, j = range.second;
    bool found = false;

    while (i != j) {
        if (i->term == term && i->region == region->id()) {
            found = true;
            break;
        }
        ++i;
    }

    size_t row = i - m_notes.begin();

    if (found) {
        // Update
        i->update(region);

        QModelIndex start = createIndex(row, 0);
        QModelIndex end = createIndex(row, NOTE_N_COLUMNS - 1);
        emit dataChanged(start, end);
    }
    else {
        if (m_notes.size() == m_limit) {
            if (row == 0)
                return;

            // Adjust animations
            for (auto &&k: m_animations)
                k.second->setData(k.first -= !!k.first);

            beginRemoveRows(QModelIndex(), 0, 0);
            m_notes.pop_front();
            endRemoveRows();
            --row;
            i = m_notes.begin() + row;
        }

        // Insert
        note.populate(term, region);

        beginInsertRows(QModelIndex(), row, row);
        m_notes.emplace(i, note);
        endInsertRows();

        // Adjust animations
        for (auto &&k: m_animations)
            if (k.first >= row)
                k.second->setData(++k.first);

        startAnimation(row);
    }
}

void
NoteModel::invalidateNotes(int64_t started)
{
    TermNote note(started);

    auto range = std::equal_range(m_notes.begin(), m_notes.end(), note);
    auto i = range.first, j = range.second;

    if (i != j) {
        size_t srow = (i - m_notes.begin());
        size_t erow = (j - m_notes.begin()) - 1;
        QModelIndex start = createIndex(srow, 0);
        QModelIndex end = createIndex(erow, NOTE_N_COLUMNS - 1);
        emit dataChanged(start, end);
    }
}

void
NoteModel::removeClosedTerminals()
{
    clearAnimations();
    int row = 0;

    for (auto i = m_notes.begin(); i != m_notes.end(); )
    {
        if (i->term == nullptr) {
            beginRemoveRows(QModelIndex(), row, row);
            i = m_notes.erase(i);
            endRemoveRows();
        } else {
            ++i;
            ++row;
        }
    }
}

/*
 * Model functions
 */
int
NoteModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NOTE_N_COLUMNS;
}

int
NoteModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notes.size();
}

bool
NoteModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && !m_notes.empty();
}

bool
NoteModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && row < m_notes.size() && column < NOTE_N_COLUMNS;
}

QModelIndex
NoteModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row < m_notes.size())
        return createIndex(row, column, (void *)&m_notes[row]);
    else
        return QModelIndex();
}

QVariant
NoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case NOTE_COLUMN_MARK:
            return tr("St", "heading");
        case NOTE_COLUMN_ROW:
            return tr("Line", "heading");
        case NOTE_COLUMN_USER:
            return tr("User", "heading");
        case NOTE_COLUMN_HOST:
            return tr("Host", "heading");
        case NOTE_COLUMN_STARTED:
            return tr("Created", "heading");
        case NOTE_COLUMN_TEXT:
            return tr("Text", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
NoteModel::data(const QModelIndex &index, int role) const
{
    const auto *note = (const TermNote *)index.internalPointer();
    if (note)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NOTE_COLUMN_MARK:
                return note->attributes[g_attr_TSQT_REGION_MARK];
            case NOTE_COLUMN_ROW:
                return note->attributes[g_attr_TSQT_REGION_ROW];
            case NOTE_COLUMN_USER:
                return note->attributes[g_attr_REGION_USER];
            case NOTE_COLUMN_HOST:
                return note->attributes[g_attr_REGION_HOST];
            case NOTE_COLUMN_STARTED:
                return note->attributes[g_attr_TSQT_REGION_STARTED];
            case NOTE_COLUMN_TEXT:
                return note->attributes[g_attr_REGION_NOTETEXT];
            default:
                return QVariant();
            }
        case Qt::BackgroundRole:
            switch (index.column()) {
            case NOTE_COLUMN_MARK:
                return note->markBg;
            default:
                return note->termBg;
            }
        case Qt::ForegroundRole:
            switch (index.column()) {
            case NOTE_COLUMN_MARK:
                return note->markFg;
            default:
                return note->termFg;
            }
        case Qt::UserRole:
            return note->fade;
        case NOTE_ROLE_ICON:
            if (index.column() == NOTE_COLUMN_TEXT)
                if (!note->attributes[g_attr_REGION_NOTETEXT].isEmpty())
                    return QVariant::fromValue((QObject*)m_renderer);
            break;
        case NOTE_ROLE_CODING:
            return QVariant::fromValue((void*)note->unicoding.get());
        case NOTE_ROLE_REGION:
            return note->region;
        case NOTE_ROLE_TERM:
            return QVariant::fromValue((QObject*)note->term);
        case NOTE_ROLE_NOTE:
            return QVariant::fromValue((void*)note);
        default:
            break;
        }

    return QVariant();
}

Qt::ItemFlags
NoteModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren;
}

//
// Filter
//
NoteFilter::NoteFilter(NoteModel *model, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_model(model)
{
    setDynamicSortFilter(false);
    setSourceModel(model);
}

void
NoteFilter::setSearchString(const QString &str)
{
    m_search = str;
    invalidateFilter();
}

void
NoteFilter::setWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.clear();
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
NoteFilter::addWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
NoteFilter::emptyWhitelist()
{
    m_whitelist.clear();
    m_haveWhitelist = true;
    invalidateFilter();
}

void
NoteFilter::addBlacklist(const Tsq::Uuid &id)
{
    m_blacklist.insert(id);
    m_haveBlacklist = true;
    invalidateFilter();
}

void
NoteFilter::resetFilter()
{
    m_whitelist.clear();
    m_blacklist.clear();
    m_haveWhitelist = m_haveBlacklist = false;
    invalidateFilter();
}

bool
NoteFilter::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = m_model->index(row, 0, parent);
    const TermNote *note = (const TermNote *)index.internalPointer();

    if (m_haveWhitelist) {
        if (!m_whitelist.contains(note->termId) && !m_whitelist.contains(note->serverId))
            return false;
    }
    else if (m_haveBlacklist) {
        if (m_blacklist.contains(note->termId) || m_blacklist.contains(note->serverId))
            return false;
    }

    return m_search.isEmpty() || note->attributes[g_attr_REGION_NOTETEXT]
        .contains(m_search, Qt::CaseInsensitive);
}

//
// View
//

#define OBJPROP_INDEX "viewHeaderIndex"

NoteView::NoteView(NoteFilter *filter, ToolWidget *parent) :
    m_filter(filter),
    m_window(parent->window())
{
    QItemSelectionModel *m = selectionModel();
    setModel(filter);
    delete m;

    setFocusPolicy(Qt::NoFocus);

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    unsigned displayMask = 0;
    displayMask |= 1 << NOTE_COLUMN_MARK;
    displayMask |= 1 << NOTE_COLUMN_USER;
    displayMask |= 1 << NOTE_COLUMN_HOST;
    displayMask |= 1 << NOTE_COLUMN_TEXT;
    m_item = new JobViewItem(displayMask, this);
    setItemDelegate(m_item);

    verticalHeader()->setVisible(false);
    m_header = horizontalHeader();
    m_header->setSectionResizeMode(QHeaderView::Interactive);
    m_header->setSectionResizeMode(NOTE_COLUMN_TEXT, QHeaderView::Stretch);
    m_header->setSectionsMovable(true);
    m_header->setContextMenuPolicy(Qt::CustomContextMenu);

    refont(g_global->jobFont());

    connect(m_window->manager(),
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*)));

    connect(m_header,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(handleHeaderContextMenu(const QPoint&)));

    connect(g_global, SIGNAL(jobFontChanged(QString)), SLOT(refont(const QString&)));

    viewport()->installEventFilter(parent);
    m_header->viewport()->installEventFilter(parent);
    verticalScrollBar()->installEventFilter(parent);
    horizontalScrollBar()->installEventFilter(parent);
}

void
NoteView::recolor(QRgb bg, QRgb fg)
{
    setStyleSheet(L("* {background-color:#%1;color:#%2}")
                  .arg(bg, 6, 16, Ch0)
                  .arg(fg, 6, 16, Ch0));
}

void
NoteView::handleTermActivated(TermInstance *term)
{
    if (m_term) {
        disconnect(m_mocTerm);
    }
    if ((m_term = term)) {
        m_mocTerm = connect(term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));
        recolor(term->bg(), term->fg());
    }
}

void
NoteView::refont(const QString &fontstr)
{
    QFont font;
    font.fromString(g_global->jobFont());
    m_item->setDisplayFont(font);
    emit m_item->sizeHintChanged(QModelIndex());

    qreal height = m_item->cellSize().height() + 2;
    verticalHeader()->setDefaultSectionSize(height);
    verticalHeader()->setVisible(false);
}

void
NoteView::action(int type, const TermNote *note)
{
    if (!note)
        return;

    QString tmp;

    switch (type) {
    case NoteActScrollStart:
        tmp = A("ScrollRegionStart|%1|%2");
        break;
    case NoteActScrollEnd:
        tmp = A("ScrollRegionEnd|%1|%2");
        break;
    case NoteActSelect:
        tmp = A("SelectJob|%1|%2");
        break;
    case NoteActRemove:
        tmp = A("RemoveNote|%1|%2");
        break;
    default:
        return;
    }

    QString arg = QString::fromLatin1(note->termId.str().c_str());
    tmp = tmp.arg(note->region).arg(arg);
    m_window->manager()->invokeSlot(tmp);
}

inline void
NoteView::mouseAction(int action)
{
    this->action(action, NOTE_NOTEP(indexAt(m_dragStartPosition)));
}

void
NoteView::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    m_clicking = true;
    QTableView::mousePressEvent(event);
}

void
NoteView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        m_clicking = dist <= QApplication::startDragDistance();
    }

    QTableView::mouseMoveEvent(event);
}

void
NoteView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->noteAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->noteAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->noteAction3());
            break;
        default:
            break;
        }
    }

    QTableView::mouseReleaseEvent(event);
}

void
NoteView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->noteAction0());

    event->accept();
}

void
NoteView::contextMenuEvent(QContextMenuEvent *event)
{
    TermNote *note = NOTE_NOTEP(indexAt(event->pos()));

    m_window->getNotePopup(note)->popup(event->globalPos());
    event->accept();
}

void
NoteView::contextMenu(QPoint point)
{
    auto sel = selectionModel()->selectedRows();
    TermNote *note = !sel.isEmpty() ? NOTE_NOTEP(sel.front()) : nullptr;

    m_window->getNotePopup(note)->popup(point);
}

void
NoteView::handleHeaderContextMenu(const QPoint &pos)
{
    if (!m_headerMenu) {
        m_headerMenu = new QMenu(this);

        for (int i = 0; i < NOTE_N_COLUMNS; ++i) {
            QVariant v = m_filter->headerData(i, Qt::Horizontal, Qt::DisplayRole);

            QAction *a = new QAction(v.toString(), m_headerMenu);
            a->setCheckable(true);
            a->setProperty(OBJPROP_INDEX, i + 1);

            connect(a, SIGNAL(triggered(bool)), SLOT(handleHeaderTriggered(bool)));
            m_headerMenu->addAction(a);
        }

        connect(m_headerMenu, SIGNAL(aboutToShow()), SLOT(preshowHeaderContextMenu()));
    }

    m_headerMenu->popup(mapToGlobal(pos));
}

void
NoteView::preshowHeaderContextMenu()
{
    for (auto i: m_headerMenu->actions())
    {
        int idx = i->property(OBJPROP_INDEX).toInt() - 1;

        if (idx >= 0)
            i->setChecked(!m_header->isSectionHidden(idx));
    }
}

void
NoteView::handleHeaderTriggered(bool checked)
{
    int idx = sender()->property(OBJPROP_INDEX).toInt() - 1;

    if (idx >= 0) {
        m_header->setSectionHidden(idx, !checked);

        // Don't permit everything to be hidden
        bool somethingShown = false;

        for (int i = 0; i < NOTE_N_COLUMNS; ++i)
            if (!m_header->isSectionHidden(i)) {
                somethingShown = true;
                break;
            }

        if (!somethingShown)
            m_header->setSectionHidden(idx, false);
    }
}

void
NoteView::setAutoscroll(bool autoscroll)
{
    if (m_autoscroll != autoscroll) {
        if ((m_autoscroll = autoscroll)) {
            connect(m_filter,
                    SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                    SLOT(handleAutoscroll()));

            scrollToBottom();
        }
        else {
            disconnect(m_filter,
                       SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                       this, SLOT(handleAutoscroll()));
        }
    }
}

void
NoteView::handleAutoscroll()
{
    QTimer::singleShot(0, this, SLOT(scrollToBottom()));
}

void
NoteView::selectFirst()
{
    if (m_filter->rowCount()) {
        QModelIndex index = m_filter->index(0, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
NoteView::selectPrevious()
{
    auto sel = selectionModel()->selectedRows();
    int row;

    if (sel.isEmpty()) {
        selectLast();
    } else if ((row = sel.front().row())) {
        QModelIndex index = m_filter->index(row - 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
NoteView::selectNext()
{
    auto sel = selectionModel()->selectedRows();
    int row;

    if (sel.isEmpty()) {
        selectFirst();
    } else if ((row = sel.front().row()) < m_filter->rowCount() - 1) {
        QModelIndex index = m_filter->index(row + 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
NoteView::selectLast()
{
    int rows = m_filter->rowCount();
    if (rows) {
        QModelIndex index = m_filter->index(rows - 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
NoteView::restoreState(int index, bool setting[3])
{
    // Restore table headers
    auto bytes = g_state->fetchVersioned(NotesColumnsKey, MODEL_VERSION, index);
    if (bytes.isEmpty() || !m_header->restoreState(bytes))
    {
        QStringList headers(DEFAULT_NOTEVIEW_COLUMNS);

        for (int i = 0; i < NOTE_N_COLUMNS; ++i) {
            bool shown = headers.contains(QString::number(i));
            m_header->setSectionHidden(i, !shown);
        }
    }

    // Restore our own settings
    bytes = g_state->fetchVersioned(NotesSettingsKey, STATE_VERSION, index, 3);
    if (!bytes.isEmpty()) {
        // Bar, header, and autoscroll settings
        setting[0] = bytes[0];
        setting[1] = bytes[1];
        setting[2] = bytes[2];
    }
}

void
NoteView::saveState(int index, const bool setting[2])
{
    QByteArray bytes;

    // Save our own settings
    bytes.append(setting[0] ? 1 : 0);
    bytes.append(setting[1] ? 1 : 0);
    bytes.append(m_autoscroll ? 1 : 0);
    g_state->storeVersioned(NotesSettingsKey, STATE_VERSION, bytes, index);

    // Save table headers
    bytes = m_header->saveState();
    g_state->storeVersioned(NotesColumnsKey, MODEL_VERSION, bytes, index);
}
