// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "app/attr.h"
#include "app/icons.h"
#include "filewidget.h"
#include "fileviewitem.h"
#include "filemodel.h"
#include "filelisting.h"
#include "filebanner.h"
#include "file.h"
#include "scrollport.h"
#include "manager.h"
#include "mainwindow.h"
#include "listener.h"
#include "server.h"
#include "term.h"
#include "menubase.h"
#include "menumacros.h"
#include "dragicon.h"
#include "settings/global.h"
#include "settings/profile.h"

#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDrag>
#include <QMimeData>
#include <QResizeEvent>

#define TR_TEXT1 TL("window-text", "No terminal is active")
#define TR_TEXT2 TL("window-text", "Directory size limit of %1 exceeded")
#define TR_TEXT3 TL("window-text", \
    "<a href='#i'>Click here</a> to raise the limit to %1 for this terminal only")
#define TR_TEXT4 TL("window-text", \
    "<a href='#e'>Edit profile %1</a> to make permanent changes")
#define TR_TEXT5 TL("window-text", "Directory error") + A(": ")

FileWidget::FileWidget(TermManager *manager) :
    SearchableWidget(manager),
    m_format(FilesLong)
{
    setAcceptDrops(true);

    m_msgLabel = new QLabel(TR_TEXT1);
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setTextFormat(Qt::RichText);
    m_msgLabel->setContextMenuPolicy(Qt::NoContextMenu);

    m_overrideFile = new TermFile;
    m_nameitem = new FileNameItem(this);
    m_model = new FileModel(manager, m_nameitem, this);
    m_filter = new FileFilter(m_model, this);
    m_longView = new FileView(m_filter, m_nameitem, this);
    auto *scroll = new FileScroll(m_nameitem);
    m_shortView = new FileListing(m_filter, m_nameitem, this, scroll);
    m_banner = new FileBanner(m_model, m_nameitem, this);
    scroll->setWidget(m_shortView);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_msgLabel);
    m_stack->addWidget(m_longView);
    m_stack->addWidget(scroll);

    auto *button = new QPushButton;
    button->setIcon(QI(ICON_CLOSE_SEARCH));
    button->installEventFilter(this);

    QHBoxLayout *lineLayout = new QHBoxLayout;
    lineLayout->setContentsMargins(g_mtmargins);
    lineLayout->addWidget(m_line, 1);
    lineLayout->addWidget(button);
    m_bar = new QWidget;
    m_bar->setLayout(lineLayout);
    m_header = m_longView->horizontalHeader();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(g_mtmargins);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_stack, 1);
    mainLayout->addWidget(m_banner);
    mainLayout->addWidget(m_bar);
    setLayout(mainLayout);

    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));

    connect(m_model, SIGNAL(termChanged(const TermDirectory*)),
            SLOT(handleTermChanged(const TermDirectory*)));
    connect(m_model, SIGNAL(directoryChanged(const TermDirectory*)),
            SLOT(handleDirectoryChanged(const TermDirectory*)));
    connect(m_model, SIGNAL(metadataChanged(const TermDirectory*)),
            m_banner, SLOT(handleInfoChanged()));

    connect(m_model, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_model, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));

    connect(m_line, SIGNAL(textChanged(const QString&)),
            m_filter, SLOT(setSearchString(const QString&)));

    connect(button, SIGNAL(clicked()),
            manager, SLOT(actionToggleToolSearchBar()));

    connect(m_msgLabel,
            SIGNAL(linkActivated(const QString&)),
            SLOT(handleLinkActivated(const QString&)));
}

FileWidget::~FileWidget()
{
    delete m_overrideFile;
}

void
FileWidget::recolor(QRgb bg, QRgb fg)
{
    setStyleSheet(L("* {background-color:#%1;color:#%2}")
                  .arg(bg, 6, 16, Ch0)
                  .arg(fg, 6, 16, Ch0));
}

void
FileWidget::updateFormat(bool force)
{
    int fmt = format();

    if ((force || fmt) && fmt != m_stack->currentIndex()) {
        switch (fmt) {
        case 1:
            m_longView->selectItem(m_selected);
            break;
        case 2:
            m_shortView->selectItem(m_selected);
            break;
        }
        m_stack->setCurrentIndex(fmt);
    }
}

void
FileWidget::updateDirectory(const TermDirectory *dir)
{
    m_banner->setDirectory(dir);

    if (dir == nullptr) {
        m_msgLabel->setText(TR_TEXT1);
        setStyleSheet(A("* {}"));
    }
    else if (dir->overlimit) {
        m_linkProfile = m_model->term()->profileName();
        m_linkLimit = QString::number(dir->limit * 10);

        QStringList msglist;
        msglist.append(TR_TEXT2.arg(dir->limit));
        msglist.append(TR_TEXT3.arg(m_linkLimit));
        msglist.append(TR_TEXT4.arg(m_linkProfile));

        m_msgLabel->setText(msglist.join(A("<br>")));
    }
    else if (dir->iserror) {
        m_msgLabel->setText(TR_TEXT5 + dir->error);
    }
    else {
        m_currentDir = dir->name;
        m_currentUrl = TermUrl::construct(m_model->server(), dir->name);
        m_selectedUrl = m_currentUrl;
        m_banner->setPath(dir->name, g_mtstr);
        m_banner->show();
        updateFormat(true);
        return;
    }

    m_currentDir.clear();
    m_currentUrl.clear();
    m_selectedUrl.clear();
    m_banner->setVisible(false);
    m_stack->setCurrentIndex(0);
}

void
FileWidget::handleTermChanged(const TermDirectory *dir)
{
    updateSelection(-1, 3);
    updateDirectory(dir);
}

void
FileWidget::handleDirectoryChanged(const TermDirectory *dir)
{
    updateSelection(-1, 4);
    updateDirectory(dir);
}

void
FileWidget::handleLinkActivated(const QString &link)
{
    if (!m_term)
        return;

    if (link == A("#i")) {
        g_listener->pushTermAttribute(m_term, TSQ_ATTR_PROFILE_NFILES, m_linkLimit);
    }
    else if (link == A("#e")) {
        m_manager->actionEditProfile(m_linkProfile);
    }
}

void
FileWidget::handleSettingsChanged()
{
    m_banner->setGitline(m_term->profile()->fileGitline());
    m_format = m_term->profile()->fileStyle();
    updateFormat(false);
}

void
FileWidget::selectNone()
{
    m_longView->selectItem(-1);
    m_shortView->selectItem(-1);
    m_selectedFile = nullptr;
    m_selected = -1;
}

void
FileWidget::selectItem(int i, const TermFile *file)
{
    m_longView->selectItem(i);
    m_shortView->selectItem(i);
    m_selectedFile = file;
    m_selected = i;
}

void
FileWidget::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    if (m_term) {
        disconnect(m_mocTerm);
        disconnect(m_mocUrl);
    }

    m_scrollport = scrollport;

    if ((m_term = term)) {
        m_mocTerm = connect(term, SIGNAL(miscSettingsChanged()),
                            SLOT(handleSettingsChanged()));
        handleSettingsChanged();

        m_mocUrl = connect(scrollport, SIGNAL(selectedUrlChanged(const TermUrl&)),
                           SLOT(handleUrlChanged(const TermUrl&)));

        if (m_selectedFile || !scrollport->selectedUrl().isEmpty())
            handleUrlChanged(scrollport->selectedUrl());
    }
}

void
FileWidget::handleUrlChanged(const TermUrl &tu)
{
    bool isfile = tu.isLocalFile() && tu.checkHost(m_model->server());
    QString path = tu.path();

    // Update banner
    if (!isfile)
        m_banner->setPath(m_currentDir, g_mtstr);
    else if (path.startsWith(m_currentDir))
        m_banner->setPath(m_currentDir, path.mid(m_currentDir.size()));
    else
        m_banner->setPath(g_mtstr, path);

    // Stop here if we caused the change
    if (m_selecting)
        return;

    selectNone();

    if (!isfile)
        return;

    if (m_currentDir == tu.dir()) {
        QString name = tu.fileName();
        for (int i = 0, n = m_filter->rowCount(); i < n; ++i) {
            const auto index = m_filter->index(i, FILE_COLUMN_NAME);
            if (index.data() == name) {
                selectItem(i, FILE_FILEP(index));
                break;
            }
        }
    }
    else {
        m_overrideFile->name = tu.fileName();
        m_overrideFile->setIsDir(tu.fileIsDir());
        m_selectedFile = m_overrideFile;
        m_selectedUrl = tu;
        m_selected = -2;
    }

    if (g_global->raiseFiles())
        m_window->filesDock()->raise();
}

void
FileWidget::updateSelection(int selected, int source)
{
    m_selected = selected;
    m_selectedFile = (selected != -1) ?
        FILE_FILEP(m_filter->index(selected, 0)) :
        nullptr;
    m_selectedUrl = TermUrl::construct(m_model->server(), m_model->dir(),
                                       m_selectedFile);

    if (m_scrollport && source != 3) {
        m_selecting = true;
        m_scrollport->setSelectedUrl(m_selectedFile ? m_selectedUrl : TermUrl());
        m_selecting = false;
    }

    if (source != 1)
        m_longView->selectItem(m_selected);
    if (source != 2)
        m_shortView->selectItem(m_selected);
}

void
FileWidget::setFormat(int format)
{
    m_formatOverride = format;
    updateFormat(false);
}

void
FileWidget::toggleFormat()
{
    m_formatOverride = (format() == FilesLong) ? FilesShort : FilesLong;
    updateFormat(false);
}

void
FileWidget::setSort(int spec)
{
    auto order = Qt::AscendingOrder;
    if (spec < 0) {
        order = Qt::DescendingOrder;
        spec = -spec;
    }
    if (spec && spec <= FILE_N_COLUMNS) {
        m_longView->sortByColumn(spec - 1, order);
    }
}

void
FileWidget::setActive(bool active)
{
    m_nameitem->setActive(active);
    m_nameitem->setBlinkEffect();
}

void
FileWidget::visibilitySet()
{
    m_nameitem->setBlinkEffect();
}

bool
FileWidget::isRemote() const
{
    return !m_term->server()->local();
}

bool
FileWidget::hasHeader() const
{
    return m_header && format() == FilesLong;
}

void
FileWidget::addDisplayActions(DynamicMenu *menu, MainWindow *parent)
{
    DynamicMenu *subMenu;
    // Note: using explicit setDynFlag on actions to skip autohide

    MS()->setDynFlag(DynFilesTool);
    SUBMENU_P("pmenu-file-sort", TN("pmenu-file", "Sort Ascending"), ICON_SORT_ASCENDING)->setDynFlag(DynFilesTool);
    for (int i = FILE_N_COLUMNS - 1; i >= 0; --i) {
        auto h = FileModel::headerData(i);
        auto t = TL("action", ACT_SORT_ASCENDING) + A(": ") + h;
        SI(h, t, ICON_SORT_ASCENDING, L("SetFileListingSort|%1").arg(i + 1));
    }

    SUBMENU_P("pmenu-file-sort", TN("pmenu-file", "Sort Descending"), ICON_SORT_DESCENDING)->setDynFlag(DynFilesTool);
    for (int i = FILE_N_COLUMNS - 1; i >= 0; --i) {
        auto h = FileModel::headerData(i);
        auto t = TL("action", ACT_SORT_DESCENDING) + A(": ") + h;
        SI(h, t, ICON_SORT_DESCENDING, L("SetFileListingSort|%1").arg(-i - 1));
    }

    QAction *a;
    auto *actionGroup = new QActionGroup(menu);

    MS()->setDynFlag(DynFilesTool);
    a = MI(TN("pmenu-file", "Short format (ls)"),
           ACT_SET_FILE_LISTING_FORMAT_SHORT, ICON_LS_FORMAT_SHORT,
           L("SetFileListingFormat|2"));

    a->setDynFlag(DynFilesToolFormatShort);
    a->setCheckable(true);
    actionGroup->addAction(a);

    a = MI(TN("pmenu-file", "Long format (ls -l)"),
           ACT_SET_FILE_LISTING_FORMAT_LONG, ICON_LS_FORMAT_LONG,
           L("SetFileListingFormat|1"));

    a->setDynFlag(DynFilesToolFormatLong);
    a->setCheckable(true);
    actionGroup->addAction(a);
}

void
FileWidget::addDisplayActions(DynamicMenu *menu)
{
    // Add display actions to the menu
    addDisplayActions(menu, m_manager->parent());

    int fmt = format();
    const auto actions = menu->actions();
    actions.at(actions.size() - 2)->setChecked(fmt != FilesLong);
    actions.at(actions.size() - 1)->setChecked(fmt == FilesLong);
}

QMenu *
FileWidget::getFilePopup(const TermFile *file)
{
    auto *server = m_model->server();
    auto tu = TermUrl::construct(server, m_currentDir, file);

    QMenu *m = m_manager->parent()->getFilePopup(server, tu);
    addDisplayActions(static_cast<DynamicMenu*>(m));
    return m;
}

QMenu *
FileWidget::getDirPopup(const QString &dir)
{
    auto *server = m_model->server();
    auto tu = TermUrl::construct(server, dir);

    QMenu *m = m_manager->parent()->getFilePopup(server, tu);
    addDisplayActions(static_cast<DynamicMenu*>(m));
    return m;
}

void
FileWidget::action(int type)
{
    auto *term = m_model->term();

    if (!term || !m_selectedUrl.hasDir())
        return;

    auto *server = term->server();
    bool islocal = server->local() && !g_global->localDownload();
    QString tmp;

    // Directory actions
    switch (type) {
    case FileActWriteDir:
        tmp = A("WriteDirectoryPath|") + m_selectedUrl.dir();
        goto out;
    case FileActCopyDir:
        tmp = A("CopyDirectoryPath|") + m_selectedUrl.dir();
        goto out;
    case FileActUploadDir:
        if (islocal)
            return;
        tmp = L("UploadToDirectory|%1|%2").arg(server->idStr(), m_selectedUrl.hostDir());
        goto out;
    default:
        if (!m_selectedUrl.hasFile())
            return;
    }

    tmp = m_selectedUrl.hostPath();

    // File actions
    switch (type) {
    case FileActSmartOpen:
        if (m_selectedUrl.fileIsDir()) {
            tmp = L("WriteTextNewline|cd %1|%2").arg(
                TermUrl::quoted(m_selectedUrl.fileName()),
                term->idStr());
            break;
        }
        // fallthru
    case FileActOpen:
        tmp = L("OpenFile||%1|%2").arg(server->idStr(), tmp);
        break;
    case FileActWriteFile:
        tmp = A("WriteFilePath|") + m_selectedUrl.path();
        break;
    case FileActCopyFile:
        tmp = A("CopyFilePath|") + m_selectedUrl.path();
        break;
    case FileActRename:
        tmp = L("RenameFile|%1|%2").arg(server->idStr(), tmp);
        break;
    case FileActDelete:
        tmp = L("DeleteFile|%1|%2").arg(server->idStr(), tmp);
        break;
    case FileActDownload:
        if (islocal)
            return;
        tmp = L("DownloadFile|%1|%2").arg(server->idStr(), tmp);
        break;
    case FileActUploadFile:
        if (islocal)
            return;
        if (m_selectedUrl.fileIsDir())
            tmp += '/';
        tmp = L("UploadFile|%1|%2").arg(server->idStr(), tmp);
        break;
    default:
        return;
    }
out:
    m_manager->invokeSlot(tmp);
}

void
FileWidget::action(int type, const QString &dir)
{
    auto *term = m_model->term();
    if (!term)
        return;

    auto *server = term->server();
    QString tmp;

    switch (type) {
    case FileActWriteDir:
    case FileActWriteFile:
        tmp = A("WriteDirectoryPath|") + dir;
        break;
    case FileActCopyDir:
    case FileActCopyFile:
        tmp = A("CopyDirectoryPath|") + dir;
        break;
    case FileActUploadDir:
        if (server->local() && !g_global->localDownload())
            return;
        tmp = L("UploadToDirectory|%1|%2").arg(server->idStr(), dir);
        break;
    case FileActSmartOpen:
        tmp = L("WriteTextNewline|cd %1|%2").arg(
            TermUrl::quoted(dir),
            term->idStr());
        break;
    case FileActOpen:
        tmp = L("OpenFile||%1|%2").arg(server->idStr(), dir);
        break;
    default:
        return;
    }

    m_manager->invokeSlot(tmp);
}

void
FileWidget::toolAction(int index)
{
    int action;

    switch (index) {
    case 0:
        action = g_global->fileAction0();
        break;
    case 1:
        action = g_global->fileAction1();
        break;
    case 2:
        action = g_global->fileAction2();
        break;
    case 3:
        action = g_global->fileAction3();
        break;
    default:
        return;
    }

    this->action(action);
}

void
FileWidget::contextMenu()
{
    QPoint mid = mapToGlobal(QPoint(width() / 2, height() / 4));
    getFilePopup(m_selectedFile)->popup(mid);
}

void
FileWidget::startDrag(const QString &path)
{
    QMimeData *data = new QMimeData;
    data->setText(path);
    data->setUrls(QList<QUrl>({ QUrl::fromLocalFile(path) }));

    QDrag *drag = new QDrag(this);
    drag->setMimeData(data);
    drag->exec(Qt::CopyAction);
}

void
FileWidget::selectFirst()
{
    if (m_filter->rowCount())
        updateSelection(0, 0);
}

void
FileWidget::selectPrevious()
{
    if (m_selected < 0) {
        selectLast();
    } else if (m_selected > 0) {
        updateSelection(m_selected - 1, 0);
    }
}

void
FileWidget::selectNext()
{
    if (m_selected < 0) {
        selectFirst();
    } else if (m_selected < m_filter->rowCount() - 1) {
        updateSelection(m_selected + 1, 0);
    }
}

void
FileWidget::selectLast()
{
    int n = m_filter->rowCount();
    if (n > 0)
        updateSelection(n - 1, 0);
}

void
FileWidget::restoreState(int index)
{
    // Returns bar and header settings
    bool setting[2] = { true, true };
    m_longView->restoreState(index, setting);

    m_bar->setVisible(m_barShown = setting[0]);
    m_header->setVisible(m_headerShown = setting[1]);
}

void
FileWidget::saveState(int index)
{
    bool setting[2] = { m_barShown, m_headerShown };
    m_longView->saveState(index, setting);
}

void
FileWidget::updateSize(const QSize &size)
{
    qreal ch = m_cellSize.height() * 5;
    qreal y = (size.height() - ch) / 2;

    if (y > 0)
        m_dragBounds.setRect(0, y, width(), ch);
    else
        m_dragBounds = rect();
}

void
FileWidget::refont(const QFont &font)
{
    if (m_font != font) {
        calculateCellSize(m_font = font);
        updateSize(size());
    }
}

void
FileWidget::resizeEvent(QResizeEvent *event)
{
    updateSize(event->size());
}

void
FileWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!m_drag)
        m_drag = new DragIcon(this);

    if (event->mimeData()->hasUrls()) {
        m_drag->setAccepted(true);
        event->acceptProposedAction();
    } else {
        m_drag->setAccepted(false);
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }

    m_drag->setGeometry(m_dragBounds);
    m_drag->raise();
    m_drag->show();
}

void
FileWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    if (m_drag)
        m_drag->hide();
}

void
FileWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        for (const auto &url: event->mimeData()->urls())
            if (url.isLocalFile())
                m_manager->actionUploadToDirectory(g_mtstr, g_mtstr, url.toLocalFile());
    } else {
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }

    if (m_drag)
        m_drag->hide();
}
