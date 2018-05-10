// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "filemodel.h"
#include "filetracker.h"
#include "filewidget.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "fileviewitem.h"
#include "infoanim.h"
#include "settings/state.h"
#include "settings/global.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QMimeData>
#include <QApplication>

#define STATE_VERSION 1
#define MODEL_VERSION 1

//
// Model
//
FileModel::FileModel(TermManager *manager, FileNameItem *nameitem, QObject *parent) :
    QAbstractTableModel(parent),
    m_nameitem(nameitem)
{
    connect(manager,
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*)));

    connect(g_listener->blink(), SIGNAL(timeout()), SLOT(handleBlinkTimer()));
}

inline void
FileModel::setBlinkEffect()
{
    m_nameitem->setBlinkSeen(m_files->blinkSeen());
    m_nameitem->setBlinkEffect();
}

inline void
FileModel::clearAnimations()
{
    forDeleteAll(m_animations);
    m_animations.clear();
}

void
FileModel::handleDirectoryChanging()
{
    clearAnimations();
    beginResetModel();

    if (m_timerId != 0)
        killTimer(m_timerId);

    m_loaded = false;
    m_timerId = startTimer(DIRLOAD_TIME);
}

void
FileModel::handleDirectoryChanged(bool attronly)
{
    if (!attronly) {
        endResetModel();
        emit directoryChanged(&m_files->dir());
        setBlinkEffect();
    } else {
        emit metadataChanged(&m_files->dir());
    }
}

void
FileModel::handleTermActivated(TermInstance *term)
{
    if (m_term) {
        m_files->disconnect(this);
        m_term->disconnect(this);
    }

    handleDirectoryChanging();
    m_nameitem->setTerm(term);

    if ((m_term = term)) {
        m_files = term->files();

        connect(m_files, SIGNAL(directoryChanging()), SLOT(handleDirectoryChanging()));
        connect(m_files, SIGNAL(directoryChanged(bool)), SLOT(handleDirectoryChanged(bool)));

        connect(m_files, SIGNAL(fileUpdated(int,unsigned)), SLOT(handleFileUpdated(int,unsigned)));
        connect(m_files, SIGNAL(fileAdding(int)), SLOT(handleFileAdding(int)));
        connect(m_files, SIGNAL(fileAdded(int)), SLOT(handleFileAdded(int)));
        connect(m_files, SIGNAL(fileRemoving(int)), SLOT(handleFileRemoving(int)));
        connect(m_files, &FileTracker::fileRemoved, this, &FileModel::endRemoveRows);

        connect(term, SIGNAL(paletteChanged()), SLOT(handlePaletteChanged()));
        connect(term, SIGNAL(colorsChanged(QRgb,QRgb)), SIGNAL(colorsChanged(QRgb,QRgb)));
        connect(term, SIGNAL(fontChanged(const QFont&)), SIGNAL(fontChanged(const QFont&)));

        endResetModel();
        emit termChanged(&m_files->dir());
        emit fontChanged(term->font());
        emit colorsChanged(term->bg(), term->fg());

        setBlinkEffect();
    }
    else {
        m_files = nullptr;
        endResetModel();
        emit termChanged(nullptr);
    }
}

void
FileModel::handlePaletteChanged()
{
    int size = m_files ? m_files->size() : 0;
    if (size) {
        QModelIndex start = createIndex(0, 0);
        QModelIndex end = createIndex(size - 1, FILE_N_COLUMNS - 1);
        emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
    }
}

void
FileModel::handleFileUpdated(int row, unsigned changes)
{
    QModelIndex start, end;

    if ((changes & ~(TermFile::FcUser|TermFile::FcGroup)) == 0) {
        start = createIndex(row, FILE_COLUMN_USER);
        end = createIndex(row, FILE_COLUMN_GROUP);
    }
    else if ((changes & ~(TermFile::FcSize|TermFile::FcMtime)) == 0) {
        start = createIndex(row, FILE_COLUMN_SIZE);
        end = createIndex(row, FILE_COLUMN_MTIME);
    }
    else if ((changes & ~TermFile::FcPerms) == 0) {
        start = createIndex(row, FILE_COLUMN_MODE);
        end = createIndex(row, FILE_COLUMN_MODE);
    }
    else {
        start = createIndex(row, 0);
        end = createIndex(row, FILE_N_COLUMNS - 1);
    }

    emit dataChanged(start, end);
    setBlinkEffect();
    startAnimation(row);
}

inline void
FileModel::adjustAnimations(int bound, int delta)
{
    const auto &c_animations = qAsConst(m_animations);
    auto j = c_animations.end();
    if (c_animations.lowerBound(bound) != j)
    {
        QMap<int,InfoAnimation*> other;

        for (auto i = c_animations.begin(); i != j; ++i)
            if (i.key() >= bound) {
                other.insert(i.key() + delta, *i);
                (*i)->setData(i.key() + delta);
            }
            else {
                other.insert(i.key(), *i);
            }

        m_animations.swap(other);
    }
}

void
FileModel::handleFileAdding(int row)
{
    beginInsertRows(QModelIndex(), row, row);

    // Adjust animation rows
    adjustAnimations(row, 1);
}

void
FileModel::handleFileAdded(int row)
{
    endInsertRows();
    setBlinkEffect();
    startAnimation(row);
}

void
FileModel::handleFileRemoving(int row)
{
    beginRemoveRows(QModelIndex(), row, row);

    // Remove any existing animation
    auto i = m_animations.find(row);
    if (i != m_animations.end()) {
        (*i)->disconnect(this);
        (*i)->deleteLater();
        m_animations.erase(i);
    }

    // Adjust animation rows
    adjustAnimations(row, -1);
}

void
FileModel::handleBlinkTimer()
{
    if (m_nameitem->updateBlink) {
        int size = m_files ? m_files->size() : 0;
        if (size) {
            QModelIndex start = createIndex(0, FILE_COLUMN_NAME);
            QModelIndex end = createIndex(size - 1, FILE_COLUMN_NAME);
            emit dataChanged(start, end, QVector<int>(1, Qt::ForegroundRole));
        }
    }
}

void
FileModel::timerEvent(QTimerEvent *)
{
    m_loaded = true;
    killTimer(m_timerId);
    m_timerId = 0;
}

void
FileModel::startAnimation(int row)
{
    if (m_loaded && m_nameitem->active() + m_files->animate() > 1 &&
        m_animations.size() < MAX_FILE_ANIMATIONS)
    {
        auto i = m_animations.constFind(row);
        if (i != m_animations.cend()) {
            (*i)->stop();
        } else {
            i = m_animations.insert(row, new InfoAnimation(this, row));
            connect(*i, SIGNAL(animationSignal(intptr_t)),
                    SLOT(handleAnimation(intptr_t)));
            connect(*i, SIGNAL(finished()), SLOT(handleAnimationFinished()));
        }
        (*i)->start();
    }
}

void
FileModel::handleAnimation(intptr_t row)
{
    if (m_files && m_files->size() > row) {
        m_animating = true;
        QModelIndex start = createIndex(row, FILE_COLUMN_NAME);
        emit dataChanged(start, start, QVector<int>(1, Qt::UserRole));
        m_animating = false;
    }
}

void
FileModel::handleAnimationFinished()
{
    auto *animation = static_cast<InfoAnimation*>(sender());

    m_animations.remove(animation->data());
    animation->deleteLater();
}

QString
FileModel::dir() const
{
    if (m_files) {
        const auto &ref = m_files->dir();

        if (!ref.iserror)
            return ref.name;
    }

    return g_mtstr;
}

ServerInstance *
FileModel::server() const
{
    return m_term ? m_term->server() : nullptr;
}

/*
 * Model functions
 */
int
FileModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : FILE_N_COLUMNS;
}

int
FileModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() && m_files ? m_files->size() : 0;
}

bool
FileModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && m_files && m_files->size();
}

bool
FileModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && m_files &&
        row < m_files->size() && column < FILE_N_COLUMNS;
}

QModelIndex
FileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && m_files && row < m_files->size())
        return createIndex(row, column, (void *)&m_files->at(row));
    else
        return QModelIndex();
}

QString
FileModel::headerData(int section)
{
    switch (section) {
    case FILE_COLUMN_MODE:
        return tr("Mode", "heading");
    case FILE_COLUMN_USER:
        return tr("User", "heading");
    case FILE_COLUMN_GROUP:
        return tr("Group", "heading");
    case FILE_COLUMN_SIZE:
        return tr("Size", "heading");
    case FILE_COLUMN_MTIME:
        return tr("Modified", "heading");
    case FILE_COLUMN_GIT:
        return tr("Git", "heading");
    case FILE_COLUMN_NAME:
        return tr("Name", "heading");
    default:
        return g_mtstr;
    }
}

QVariant
FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (role == Qt::DisplayRole && orientation == Qt::Horizontal) ?
        headerData(section) :
        QVariant();
}

QVariant
FileModel::data(const QModelIndex &index, int role) const
{
    const auto *file = (const TermFile *)index.internalPointer();
    if (file)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case FILE_COLUMN_MODE:
                return file->modestr;
            case FILE_COLUMN_USER:
                return file->user;
            case FILE_COLUMN_GROUP:
                return file->group;
            case FILE_COLUMN_SIZE:
                return QString::number(file->size);
            case FILE_COLUMN_MTIME:
                return file->mtimestr;
            case FILE_COLUMN_NAME:
                return file->name;
            default:
                return QVariant();
            }
        case Qt::UserRole: {
            const auto i = m_animations.constFind(index.row());
            return i != m_animations.cend() ? (*i)->fade() : 0;
        }
        case FILE_ROLE_FILE:
            return QVariant::fromValue((void *)file);
        case FILE_ROLE_TERM:
            return QVariant::fromValue((QObject *)m_term);
        default:
            break;
        }

    return QVariant();
}

Qt::ItemFlags
FileModel::flags(const QModelIndex &index) const
{
    auto rc = Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren;

    if (index.column() == FILE_COLUMN_NAME)
        rc |= Qt::ItemIsDragEnabled;

    return rc;
}

QStringList
FileModel::mimeTypes() const
{
    auto result = QAbstractItemModel::mimeTypes();
    result.append(A("text/plain"));
    result.append(A("text/uri-list"));
    return result;
}

QMimeData *
FileModel::mimeData(const QModelIndexList &indexes) const
{
    QString dirname = m_files->dir().name;
    QStringList text;
    QList<QUrl> urls;

    for (auto &i: indexes) {
        const auto *file = (const TermFile *)i.internalPointer();
        QString link = dirname + file->name;
        text.append(link);
        urls.append(QUrl::fromLocalFile(link));
    }

    auto *result = QAbstractItemModel::mimeData(indexes);
    result->setText(text.join('\n'));
    result->setUrls(urls);
    return result;
}

bool
FileModel::dropMimeData(const QMimeData*, Qt::DropAction, int, int, const QModelIndex&)
{
    return false;
}

//
// Filter
//
FileFilter::FileFilter(FileModel *model, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_model(model)
{
    setSourceModel(model);
}

void
FileFilter::setSearchString(const QString &str)
{
    m_search = str;
    invalidateFilter();
}

bool
FileFilter::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const FileTracker *files = m_model->files();
    const QString &name = files->at(row).name;

    if (!files->showDotfiles() && name.startsWith('.'))
        return false;
    if (m_search.isEmpty())
        return true;

    return name.contains(m_search, Qt::CaseInsensitive);
}

bool
FileFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto *lfile = (const TermFile *)left.internalPointer();
    const auto *rfile = (const TermFile *)right.internalPointer();

    switch (left.column()) {
    case FILE_COLUMN_MTIME:
        if (lfile->mtime != rfile->mtime)
            return lfile->mtime < rfile->mtime;
        break;
    case FILE_COLUMN_SIZE:
        if (lfile->size != rfile->size)
            return lfile->size < rfile->size;
        break;
    case FILE_COLUMN_MODE:
        if (lfile->mode != rfile->mode)
            return lfile->mode < rfile->mode;
        break;
    case FILE_COLUMN_GIT:
        if (lfile->gitflags != rfile->gitflags)
            return lfile->gitflags < rfile->gitflags;
        break;
    case FILE_COLUMN_NAME:
        break;
    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }

    return lfile->name.compare(rfile->name, Qt::CaseInsensitive) < 0;
}

//
// View
//
#define OBJPROP_INDEX "viewHeaderIndex"

FileView::FileView(FileFilter *filter, FileNameItem *item, FileWidget *parent) :
    m_filter(filter),
    m_nameitem(item),
    m_parent(parent)
{
    QItemSelectionModel *m = selectionModel();
    setModel(filter);
    delete m;

    setFocusPolicy(Qt::NoFocus);

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(FILE_COLUMN_NAME, Qt::AscendingOrder);

    unsigned displayMask = 0;
    displayMask |= 1 << FILE_COLUMN_USER;
    displayMask |= 1 << FILE_COLUMN_GROUP;
    displayMask |= 1 << FILE_COLUMN_NAME;
    m_viewitem = new FileViewItem(displayMask, this);
    setItemDelegate(m_viewitem);
    setItemDelegateForColumn(FILE_COLUMN_GIT, m_nameitem);
    setItemDelegateForColumn(FILE_COLUMN_NAME, m_nameitem);

    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    verticalHeader()->setVisible(false);
    m_header = horizontalHeader();
    m_header->setSectionResizeMode(QHeaderView::Interactive);
    m_header->setSectionResizeMode(FILE_COLUMN_NAME, QHeaderView::Stretch);
    m_header->setSectionsMovable(true);
    m_header->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_header,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(handleHeaderContextMenu(const QPoint&)));

    connect(filter->model(), SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));

    connect(selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleActivity()));

    viewport()->installEventFilter(parent);
    m_header->viewport()->installEventFilter(parent);
    verticalScrollBar()->installEventFilter(parent);
    horizontalScrollBar()->installEventFilter(parent);
}

void
FileView::handleActivity()
{
    m_nameitem->setBlinkEffect();
}

void
FileView::scrollContentsBy(int dx, int dy)
{
    m_nameitem->setBlinkEffect();
    QTableView::scrollContentsBy(dx, dy);
}

void
FileView::refont(const QFont &font)
{
    m_nameitem->setDisplayFont(font);
    m_viewitem->setDisplayFont(font);

    verticalHeader()->setDefaultSectionSize(m_viewitem->cellSize().height());
    verticalHeader()->setVisible(false);
}

inline void
FileView::mouseAction(int action)
{
    m_parent->action(action);
}

void
FileView::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    m_clicking = true;
    QTableView::mousePressEvent(event);
    const auto index = indexAt(m_dragStartPosition);
    int i = index.isValid() ? index.row() : -1;

    if (i == -1)
        selectionModel()->clear();

    m_parent->updateSelection(i, 1);

    m_nameitem->setBlinkEffect();
}

void
FileView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        m_clicking = dist <= QApplication::startDragDistance();
    }

    QTableView::mouseMoveEvent(event);
}

void
FileView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->fileAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->fileAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->fileAction3());
            break;
        default:
            break;
        }
    }

    QTableView::mouseReleaseEvent(event);
}

void
FileView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->fileAction0());

    event->accept();
}

void
FileView::contextMenuEvent(QContextMenuEvent *event)
{
    TermFile *file = FILE_FILEP(indexAt(event->pos()));

    m_parent->getFilePopup(file)->popup(event->globalPos());
    event->accept();
}

void
FileView::handleHeaderContextMenu(const QPoint &pos)
{
    if (!m_headerMenu) {
        m_headerMenu = new QMenu(this);

        for (int i = 0; i < FILE_N_COLUMNS; ++i) {
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
FileView::preshowHeaderContextMenu()
{
    for (auto i: m_headerMenu->actions())
    {
        int idx = i->property(OBJPROP_INDEX).toInt() - 1;

        if (idx >= 0)
            i->setChecked(!m_header->isSectionHidden(idx));
    }
}

void
FileView::handleHeaderTriggered(bool checked)
{
    int idx = sender()->property(OBJPROP_INDEX).toInt() - 1;

    if (idx >= 0) {
        m_header->setSectionHidden(idx, !checked);

        // Don't permit everything to be hidden
        bool somethingShown = false;

        for (int i = 0; i < FILE_N_COLUMNS; ++i)
            if (!m_header->isSectionHidden(i)) {
                somethingShown = true;
                break;
            }

        if (!somethingShown)
            m_header->setSectionHidden(idx, false);
    }
}

void
FileView::selectItem(int i)
{
    if (i < 0) {
        selectionModel()->clear();
    } else {
        selectRow(i);
        scrollTo(m_filter->index(i, 0));
    }
}

void
FileView::restoreState(int index, bool setting[2])
{
    // Restore table headers
    auto bytes = g_state->fetchVersioned(FilesColumnsKey, MODEL_VERSION, index);
    if (bytes.isEmpty() || !m_header->restoreState(bytes))
    {
        QStringList headers(DEFAULT_FILEVIEW_COLUMNS);

        for (int i = 0; i < FILE_N_COLUMNS; ++i) {
            bool shown = headers.contains(QString::number(i));
            m_header->setSectionHidden(i, !shown);
        }
    }

    // Restore our own settings
    bytes = g_state->fetchVersioned(FilesSettingsKey, STATE_VERSION, index, 2);
    if (!bytes.isEmpty()) {
        // Bar and header settings
        setting[0] = bytes[0];
        setting[1] = bytes[1];
    }
}

void
FileView::saveState(int index, const bool setting[2])
{
    QByteArray bytes;

    // Save our own settings
    bytes.append(setting[0] ? 1 : 0);
    bytes.append(setting[1] ? 1 : 0);
    g_state->storeVersioned(FilesSettingsKey, STATE_VERSION, bytes, index);

    // Save table headers
    bytes = m_header->saveState();
    g_state->storeVersioned(FilesColumnsKey, MODEL_VERSION, bytes, index);
}
