// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/datastore.h"
#include "jobmodel.h"
#include "jobviewitem.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "dockwidget.h"
#include "mainwindow.h"
#include "infoanim.h"
#include "settings/global.h"
#include "settings/state.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMimeData>
#include <QApplication>
#include <QTimer>
#include <algorithm>

#define STATE_VERSION 1
#define MODEL_VERSION 1

//
// Model
//
JobModel::JobModel(TermListener *parent) : QAbstractTableModel(parent)
{
    connect(parent, SIGNAL(termAdded(TermInstance*)), SLOT(handleTermAdded(TermInstance*)));
    connect(parent, SIGNAL(termRemoved(TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));

    connect(g_global, SIGNAL(settingsLoaded()), SLOT(relimit()));
    m_limit = g_global->jobLimit();
}

void
JobModel::clearAnimations()
{
    for (auto &i: qAsConst(m_animations)) {
        m_jobs[i.first].fade = 0;
        i.second->deleteLater();
    }

    m_animations.clear();
}

void
JobModel::startAnimation(size_t row)
{
    for (auto &i: qAsConst(m_animations)) {
        if (i.first == row) {
            i.second->stop();
            i.second->start();
            return;
        }
    }
    if (m_animations.size() < MAX_JOB_ANIMATIONS) {
        auto *animation = new InfoAnimation(this, row);
        m_animations.push_back(std::make_pair(row, animation));
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
        connect(animation, SIGNAL(finished()), SLOT(handleAnimationFinished()));
        animation->start();
    }
}

void
JobModel::handleAnimation(intptr_t row)
{
    auto *animation = static_cast<InfoAnimation*>(sender());
    m_jobs[row].fade = animation->fade();

    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, JOB_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::UserRole));
}

void
JobModel::handleAnimationFinished()
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
JobModel::relimit()
{
    size_t limit = g_global->jobLimit();

    if (m_limit != limit) {
        if (m_limit > limit) {
            clearAnimations();
            beginResetModel();
            while (m_jobs.size() > limit)
                m_jobs.pop_front();
            endResetModel();
        }
        m_limit = limit;
    }
}

void
JobModel::handleTermAdded(TermInstance *term)
{
    connect(term->buffers(), SIGNAL(jobChanged(TermInstance*,Region*)), SLOT(handleJobChanged(TermInstance*,Region*)));
    connect(term, SIGNAL(paletteChanged()), SLOT(handleTermChanged()));
    connect(term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(handleTermChanged()));
}

void
JobModel::handleTermChanged()
{
    QObject *term = sender();

    QModelIndex start;
    int row = 0;
    bool flag = false;

    for (auto &job: m_jobs)
    {
        if (job.term == term) {
            job.recolor(job.attributes.value(g_attr_REGION_EXITCODE));
            if (!flag) {
                start = createIndex(row, 0);
                flag = true;
            }
        } else if (flag) {
            emit dataChanged(start, createIndex(row, JOB_N_COLUMNS - 1));
            flag = false;
        }

        ++row;
    }

    if (flag)
        emit dataChanged(start, createIndex(row - 1, JOB_N_COLUMNS - 1));
}

void
JobModel::handleTermRemoved(TermInstance *term)
{
    QModelIndex start;
    int row = 0;
    bool flag = false;

    for (auto &job: m_jobs)
    {
        if (job.term == term) {
            job.term = nullptr;
            if (!flag) {
                start = createIndex(row, 0);
                flag = true;
            }
        } else if (flag) {
            emit dataChanged(start, createIndex(row, JOB_N_COLUMNS - 1));
            flag = false;
        }

        ++row;
    }

    if (flag)
        emit dataChanged(start, createIndex(row - 1, JOB_N_COLUMNS - 1));
}

void
JobModel::handleJobChanged(TermInstance *term, Region *region)
{
    int64_t started = region->attributes[g_attr_REGION_STARTED].toLongLong();
    TermJob job(started);

    auto range = std::equal_range(m_jobs.begin(), m_jobs.end(), job);
    auto i = range.first, j = range.second;
    bool found = false;

    while (i != j) {
        if (i->term == term && i->region == region->id()) {
            found = true;
            break;
        }
        ++i;
    }

    size_t row = i - m_jobs.begin();

    if (found) {
        // Update
        if (i->update(region)) {
            // Store
            g_datastore->storeCommand(i->attributes);
            startAnimation(row);
            // Report
            term->reportJob(&*i);
        }

        QModelIndex start = createIndex(row, 0);
        QModelIndex end = createIndex(row, JOB_N_COLUMNS - 1);
        emit dataChanged(start, end);
    }
    else {
        if (m_jobs.size() == m_limit) {
            if (row == 0)
                return;

            // Adjust animations
            for (auto &&k: m_animations)
                k.second->setData(k.first -= !!k.first);

            beginRemoveRows(QModelIndex(), 0, 0);
            m_jobs.pop_front();
            endRemoveRows();
            --row;
            i = m_jobs.begin() + row;
        }

        // Insert
        job.populate(term, region);

        beginInsertRows(QModelIndex(), row, row);
        m_jobs.emplace(i, job);
        endInsertRows();

        // Adjust animations
        for (auto &&k: m_animations)
            if (k.first >= row)
                k.second->setData(++k.first);

        startAnimation(row);
    }
}

void
JobModel::invalidateJobs(int64_t started)
{
    TermJob job(started);

    auto range = std::equal_range(m_jobs.begin(), m_jobs.end(), job);
    auto i = range.first, j = range.second;

    if (i != j) {
        size_t srow = (i - m_jobs.begin());
        size_t erow = (j - m_jobs.begin()) - 1;
        QModelIndex start = createIndex(srow, 0);
        QModelIndex end = createIndex(erow, JOB_N_COLUMNS - 1);
        emit dataChanged(start, end);
    }
}

void
JobModel::removeClosedTerminals()
{
    clearAnimations();
    int row = 0;

    for (auto i = m_jobs.begin(); i != m_jobs.end(); )
    {
        if (i->term == nullptr) {
            beginRemoveRows(QModelIndex(), row, row);
            i = m_jobs.erase(i);
            endRemoveRows();
        } else {
            ++i;
            ++row;
        }
    }
}

QString
JobModel::findCommand(const Tsq::Uuid &terminalId, regionid_t regionId) const
{
    for (const auto &job: m_jobs)
        if (job.termId == terminalId && job.region == regionId)
            return job.attributes.value(g_attr_REGION_COMMAND);

    return g_mtstr;
}

/*
 * Model functions
 */
int
JobModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : JOB_N_COLUMNS;
}

int
JobModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_jobs.size();
}

bool
JobModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && !m_jobs.empty();
}

bool
JobModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && row < m_jobs.size() && column < JOB_N_COLUMNS;
}

QModelIndex
JobModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row < m_jobs.size())
        return createIndex(row, column, (void *)&m_jobs[row]);
    else
        return QModelIndex();
}

QVariant
JobModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case JOB_COLUMN_MARK:
            return tr("St", "heading");
        case JOB_COLUMN_ROW:
            return tr("Line", "heading");
        case JOB_COLUMN_USER:
            return tr("User", "heading");
        case JOB_COLUMN_HOST:
            return tr("Host", "heading");
        case JOB_COLUMN_PATH:
            return tr("Directory", "heading");
        case JOB_COLUMN_STARTED:
            return tr("Started", "heading");
        case JOB_COLUMN_DURATION:
            return tr("Duration", "heading");
        case JOB_COLUMN_COMMAND:
            return tr("Command", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
JobModel::data(const QModelIndex &index, int role) const
{
    const auto *job = (const TermJob *)index.internalPointer();
    if (job)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case JOB_COLUMN_MARK:
                return job->attributes[g_attr_TSQT_REGION_MARK];
            case JOB_COLUMN_ROW:
                return job->attributes[g_attr_TSQT_REGION_ROW];
            case JOB_COLUMN_USER:
                return job->attributes[g_attr_REGION_USER];
            case JOB_COLUMN_HOST:
                return job->attributes[g_attr_REGION_HOST];
            case JOB_COLUMN_PATH:
                return job->attributes[g_attr_REGION_PATH];
            case JOB_COLUMN_STARTED:
                return job->attributes[g_attr_TSQT_REGION_STARTED];
            case JOB_COLUMN_DURATION:
                return job->attributes[g_attr_TSQT_REGION_DURATION];
            case JOB_COLUMN_COMMAND:
                return job->attributes[g_attr_REGION_COMMAND];
            default:
                return QVariant();
            }
        case Qt::BackgroundRole:
            switch (index.column()) {
            case JOB_COLUMN_MARK:
                return job->markBg;
            default:
                return job->commandBg;
            }
        case Qt::ForegroundRole:
            switch (index.column()) {
            case JOB_COLUMN_MARK:
                return job->markFg;
            default:
                return job->commandFg;
            }
        case Qt::UserRole:
            return job->fade;
        case JOB_ROLE_ICON:
            if (index.column() == JOB_COLUMN_COMMAND)
                return QVariant::fromValue((QObject*)job->renderer);
            break;
        case JOB_ROLE_CODING:
            return QVariant::fromValue((void*)job->unicoding.get());
        case JOB_ROLE_REGION:
            return job->region;
        case JOB_ROLE_TERM:
            return QVariant::fromValue((QObject*)job->term);
        case JOB_ROLE_JOB:
            return QVariant::fromValue((void*)job);
        default:
            break;
        }

    return QVariant();
}

Qt::ItemFlags
JobModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|
        Qt::ItemNeverHasChildren|Qt::ItemIsDragEnabled;
}

QStringList
JobModel::mimeTypes() const
{
    auto result = QAbstractItemModel::mimeTypes();
    result.append(A("text/plain"));
    return result;
}

QMimeData *
JobModel::mimeData(const QModelIndexList &indexes) const
{
    auto *result = QAbstractItemModel::mimeData(indexes);

    if (!indexes.isEmpty()) {
        auto job = (const TermJob *)indexes.at(0).internalPointer();
        if (job)
            result->setText(job->attributes.value(g_attr_REGION_COMMAND));
    }

    return result;
}

bool
JobModel::dropMimeData(const QMimeData*, Qt::DropAction, int, int, const QModelIndex&)
{
    return false;
}

//
// Filter
//
JobFilter::JobFilter(JobModel *model, ToolWidget *parent) :
    QSortFilterProxyModel(parent),
    m_manager(parent->manager()),
    m_model(model)
{
    setDynamicSortFilter(false);
    setSourceModel(model);

    connect(m_manager,
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));

    handleTermActivated(m_manager->activeTerm(), m_manager->activeScrollport());
}

void
JobFilter::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    if (m_scrollport) {
        disconnect(m_mocTerm);
    }
    if ((m_scrollport = scrollport)) {
        m_mocTerm = connect(m_scrollport, SIGNAL(activeJobChanged(regionid_t)),
                            SLOT(handleActiveJobChanged(regionid_t)));

        handleActiveJobChanged(scrollport->activeJobId());
    } else {
        handleActiveJobChanged(INVALID_REGION_ID);
    }
}

void
JobFilter::handleActiveJobChanged(regionid_t activeJobId)
{
    m_model->invalidateJobs(m_activeJobStarted);

    if (m_scrollport) {
        const Region *r = m_scrollport->buffers()->safeRegion(activeJobId);
        if (r) {
            auto i = r->attributes.constFind(g_attr_REGION_STARTED);
            if (i != r->attributes.cend()) {
                m_activeJobStarted = i->toLongLong();
                m_model->invalidateJobs(m_activeJobStarted);
            }
        }
    }
}

QVariant
JobFilter::data(const QModelIndex &index, int role) const
{
    if (index.column() == JOB_COLUMN_MARK &&
        (role == Qt::BackgroundRole || role == Qt::ForegroundRole))
    {
        const TermJob *job = JOB_JOBP(index);
        const TermInstance *term = job->term;

        if (m_scrollport &&
            m_scrollport->activeJobId() == job->region &&
            m_scrollport->term() == term)
        {
            QRgb rgb = term->palette().at((role == Qt::ForegroundRole) ?
                                          PALETTE_SH_MARK_SELECTED_FG :
                                          PALETTE_SH_MARK_SELECTED_BG);
            if (PALETTE_IS_ENABLED(rgb))
                return rgb;
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

void
JobFilter::setSearchString(const QString &str)
{
    m_search = str;
    invalidateFilter();
}

void
JobFilter::setWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.clear();
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
JobFilter::addWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
JobFilter::emptyWhitelist()
{
    m_whitelist.clear();
    m_haveWhitelist = true;
    invalidateFilter();
}

void
JobFilter::addBlacklist(const Tsq::Uuid &id)
{
    m_blacklist.insert(id);
    m_haveBlacklist = true;
    invalidateFilter();
}

void
JobFilter::resetFilter()
{
    m_whitelist.clear();
    m_blacklist.clear();
    m_haveWhitelist = m_haveBlacklist = false;
    invalidateFilter();
}

bool
JobFilter::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = m_model->index(row, 0, parent);
    const TermJob *job = (const TermJob *)index.internalPointer();

    if (m_haveWhitelist) {
        if (!m_whitelist.contains(job->termId) && !m_whitelist.contains(job->serverId))
            return false;
    }
    else if (m_haveBlacklist) {
        if (m_blacklist.contains(job->termId) || m_blacklist.contains(job->serverId))
            return false;
    }

    return m_search.isEmpty() || job->attributes[g_attr_REGION_COMMAND]
        .contains(m_search, Qt::CaseInsensitive);
}

//
// View
//

#define OBJPROP_INDEX "viewHeaderIndex"

JobView::JobView(JobFilter *filter, ToolWidget *parent) :
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
    displayMask |= 1 << JOB_COLUMN_MARK;
    displayMask |= 1 << JOB_COLUMN_USER;
    displayMask |= 1 << JOB_COLUMN_HOST;
    displayMask |= 1 << JOB_COLUMN_PATH;
    displayMask |= 1 << JOB_COLUMN_COMMAND;
    m_item = new JobViewItem(displayMask, this);
    setItemDelegate(m_item);

    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    verticalHeader()->setVisible(false);
    m_header = horizontalHeader();
    m_header->setSectionResizeMode(QHeaderView::Interactive);
    m_header->setSectionResizeMode(JOB_COLUMN_COMMAND, QHeaderView::Stretch);
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
JobView::recolor(QRgb bg, QRgb fg)
{
    setStyleSheet(L("* {background-color:#%1;color:#%2}")
                  .arg(bg, 6, 16, Ch0)
                  .arg(fg, 6, 16, Ch0));
}

void
JobView::handleTermActivated(TermInstance *term)
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
JobView::refont(const QString &fontstr)
{
    QFont font;
    font.fromString(g_global->jobFont());
    m_item->setDisplayFont(font);
    emit m_item->sizeHintChanged(QModelIndex());

    qreal height = m_item->cellSize().height();
    verticalHeader()->setDefaultSectionSize(height);
    verticalHeader()->setVisible(false);
}

void
JobView::action(int type, const TermJob *job)
{
    if (!job)
        return;

    QString tmp;

    switch (type) {
    case JobActScrollStart:
        tmp = A("ScrollRegionStart|%1|%2");
        break;
    case JobActScrollEnd:
        tmp = A("ScrollRegionEnd|%1|%2");
        break;
    case JobActCopyCommand:
        tmp = A("CopyCommand|%1|%2");
        break;
    case JobActWriteCommand:
        tmp = A("WriteCommand|%1|%2");
        break;
    case JobActWriteCommandNewline:
        tmp = A("WriteCommandNewline|%1|%2");
        break;
    case JobActCopyOutput:
        tmp = A("CopyOutput|%1|%2");
        break;
    case JobActCopy:
        tmp = A("CopyJob|%1|%2");
        break;
    case JobActSelectOutput:
        tmp = A("SelectOutput|%1|%2");
        break;
    case JobActSelectCommand:
        tmp = A("SelectCommand|%1|%2");
        break;
    case JobActSelect:
        tmp = A("SelectJob|%1|%2");
        break;
    default:
        return;
    }

    QString arg = QString::fromLatin1(job->termId.str().c_str());
    tmp = tmp.arg(job->region).arg(arg);
    m_window->manager()->invokeSlot(tmp);
}

inline void
JobView::mouseAction(int action)
{
    this->action(action, JOB_JOBP(indexAt(m_dragStartPosition)));
}

void
JobView::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    m_clicking = true;
    QTableView::mousePressEvent(event);
}

void
JobView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        m_clicking = dist <= QApplication::startDragDistance();
    }

    QTableView::mouseMoveEvent(event);
}

void
JobView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->jobAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->jobAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->jobAction3());
            break;
        default:
            break;
        }
    }

    QTableView::mouseReleaseEvent(event);
}

void
JobView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->jobAction0());

    event->accept();
}

void
JobView::contextMenuEvent(QContextMenuEvent *event)
{
    TermJob *job = JOB_JOBP(indexAt(event->pos()));

    m_window->getJobPopup(job)->popup(event->globalPos());
    event->accept();
}

void
JobView::contextMenu(QPoint point)
{
    auto sel = selectionModel()->selectedRows();
    TermJob *job = !sel.isEmpty() ? JOB_JOBP(sel.front()) : nullptr;

    m_window->getJobPopup(job)->popup(point);
}

void
JobView::handleHeaderContextMenu(const QPoint &pos)
{
    if (!m_headerMenu) {
        m_headerMenu = new QMenu(this);

        for (int i = 0; i < JOB_N_COLUMNS; ++i) {
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
JobView::preshowHeaderContextMenu()
{
    for (auto i: m_headerMenu->actions())
    {
        int idx = i->property(OBJPROP_INDEX).toInt() - 1;

        if (idx >= 0)
            i->setChecked(!m_header->isSectionHidden(idx));
    }
}

void
JobView::handleHeaderTriggered(bool checked)
{
    int idx = sender()->property(OBJPROP_INDEX).toInt() - 1;

    if (idx >= 0) {
        m_header->setSectionHidden(idx, !checked);

        // Don't permit everything to be hidden
        bool somethingShown = false;

        for (int i = 0; i < JOB_N_COLUMNS; ++i)
            if (!m_header->isSectionHidden(i)) {
                somethingShown = true;
                break;
            }

        if (!somethingShown)
            m_header->setSectionHidden(idx, false);
    }
}

void
JobView::setAutoscroll(bool autoscroll)
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
JobView::handleAutoscroll()
{
    QTimer::singleShot(0, this, SLOT(scrollToBottom()));
}

void
JobView::selectFirst()
{
    if (m_filter->rowCount()) {
        QModelIndex index = m_filter->index(0, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
JobView::selectPrevious()
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
JobView::selectNext()
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
JobView::selectLast()
{
    int rows = m_filter->rowCount();
    if (rows) {
        QModelIndex index = m_filter->index(rows - 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
JobView::restoreState(int index, bool setting[3])
{
    // Restore table headers
    auto bytes = g_state->fetchVersioned(JobsColumnsKey, MODEL_VERSION, index);
    if (bytes.isEmpty() || !m_header->restoreState(bytes))
    {
        QStringList headers(DEFAULT_JOBVIEW_COLUMNS);

        for (int i = 0; i < JOB_N_COLUMNS; ++i) {
            bool shown = headers.contains(QString::number(i));
            m_header->setSectionHidden(i, !shown);
        }
    }

    // Restore our own settings
    bytes = g_state->fetchVersioned(JobsSettingsKey, STATE_VERSION, index, 3);
    if (!bytes.isEmpty()) {
        // Bar, header, and autoscroll settings
        setting[0] = bytes[0];
        setting[1] = bytes[1];
        setting[2] = bytes[2];
    }
}

void
JobView::saveState(int index, const bool setting[2])
{
    QByteArray bytes;

    // Save our own settings
    bytes.append(setting[0] ? 1 : 0);
    bytes.append(setting[1] ? 1 : 0);
    bytes.append(m_autoscroll ? 1 : 0);
    g_state->storeVersioned(JobsSettingsKey, STATE_VERSION, bytes, index);

    // Save table headers
    bytes = m_header->saveState();
    g_state->storeVersioned(JobsColumnsKey, MODEL_VERSION, bytes, index);
}
