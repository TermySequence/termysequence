// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "mainwindow.h"
#include "menubase.h"
#include "listener.h"
#include "manager.h"
#include "server.h"
#include "term.h"
#include "mainwidget.h"
#include "dockwidget.h"
#include "barwidget.h"
#include "keymapwidget.h"
#include "suggestwidget.h"
#include "searchwidget.h"
#include "jobwidget.h"
#include "notewidget.h"
#include "taskwidget.h"
#include "taskmodel.h"
#include "filewidget.h"
#include "statusarrange.h"
#include "statuslink.h"
#include "task.h"
#include "settings/global.h"
#include "settings/keymap.h"
#include "settings/state.h"
#include "settings/orderlayout.h"
#include "setup/systemdsetup.h"

#include <QSignalMapper>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QDesktopWidget>

#define TR_TAB1 TL("tab-title", "Terminals")
#define TR_TAB2 TL("tab-title", "Keymap")
#define TR_TAB3 TL("tab-title", "Suggestions")
#define TR_TAB4 TL("tab-title", "Files")
#define TR_TAB5 TL("tab-title", "Tasks")
#define TR_TAB6 TL("tab-title", "Search")
#define TR_TAB7 TL("tab-title", "History")
#define TR_TAB8 TL("tab-title", "Annotations")
#define TR_TEXT1 TL("window-text", "Welcome to %1, version %2")
//#define TR_TEXT2 TL("window-text", "%1 has no shortcut in keymap %2")
#define TR_TEXT3 TL("window-text", "%1 maps to %2 in keymap %3")
#define TR_TEXT5 TL("window-text", "Not connected")
#define TR_TEXT6 TL("window-text", "Select a connection option from the Connect menu")

static int s_nextIndex;

MainWindow::MainWindow(TermManager *manager) :
    m_manager(manager),
    m_index(s_nextIndex++)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_manager->setParent(this);

    a_statusTipForwarder = new QAction(this);
    m_slotMapper = new QSignalMapper(this);
    connect(m_slotMapper, SIGNAL(mapped(const QString&)), SLOT(menuAction(const QString&)));
    createMenus();

    OrderLayoutReader order;
    if (g_global->restoreOrder()) {
        order.parse(g_state->fetch(WindowOrderKey, m_index));
        m_manager->loadOrder(order);
    }

    setCentralWidget(m_widget = new MainWidget(m_manager, this));

    statusBar()->addPermanentWidget(new QLabel(C('|')));
    statusBar()->addPermanentWidget(new StatusLink(this));
    statusBar()->addWidget(m_status = new MainStatus(m_manager), 1);

    m_terminals = new DockWidget(TR_TAB1, a_dockTerminals, 0, this);
    m_terminals->setWidget(new BarScroll(m_manager));
    m_terminals->setObjectName("terminals");
    addDockWidget(Qt::TopDockWidgetArea, m_terminals);

    m_keymap = new DockWidget(TR_TAB2, a_dockKeymap, 0, this);
    m_keymap->setWidget(new KeymapWidget(m_manager));
    m_keymap->setObjectName("keymap");
    tabifyDockWidget(m_terminals, m_keymap);

    m_commands = new DockWidget(TR_TAB3, a_dockCommands, DynSuggestionsTool, this);
    m_commandsWidget = new SuggestWidget(m_manager);
    m_commands->setToolWidget(m_commandsWidget, false, false);
    m_commands->setObjectName("suggestions");
    tabifyDockWidget(m_keymap, m_commands);

    m_filesWidget = new FileWidget(m_manager);
    m_filesWidget->restoreState(m_index);
    m_files = new DockWidget(TR_TAB4, a_dockFiles, DynFilesTool, this);
    m_files->setToolWidget(m_filesWidget, true, false);
    m_files->setObjectName("files");
    tabifyDockWidget(m_commands, m_files);

    m_tasksWidget = new TaskWidget(m_manager, g_listener->taskmodel());
    m_tasksWidget->restoreState(m_index);
    m_tasks = new DockWidget(TR_TAB5, a_dockTasks, DynTasksTool, this);
    m_tasks->setToolWidget(m_tasksWidget, false, true);
    m_tasks->setObjectName("tasks");
    tabifyDockWidget(m_files, m_tasks);

    m_search = new DockWidget(TR_TAB6, a_dockSearch, DynSearchTool, this);
    m_searchWidget = new SearchWidget(m_manager);
    m_searchWidget->restoreState(m_index);
    m_search->setToolWidget(m_searchWidget, true, false);
    m_search->setObjectName("search");
    tabifyDockWidget(m_tasks, m_search);
    m_search->hide();

    m_jobsWidget = new JobWidget(m_manager, g_listener->jobmodel());
    m_jobsWidget->restoreState(m_index);
    m_jobs = new DockWidget(TR_TAB7, a_dockJobs, DynHistoryTool, this);
    m_jobs->setToolWidget(m_jobsWidget, true, true);
    m_jobs->setObjectName("history");
    addDockWidget(Qt::RightDockWidgetArea, m_jobs);
    m_jobs->hide();

    m_notesWidget = new NoteWidget(m_manager, g_listener->notemodel());
    m_notesWidget->restoreState(m_index);
    m_notes = new DockWidget(TR_TAB8, a_dockNotes, DynNotesTool, this);
    m_notes->setToolWidget(m_notesWidget, true, true);
    m_notes->setObjectName("notes");
    addDockWidget(Qt::RightDockWidgetArea, m_notes);
    m_notes->hide();

    m_terminals->raise();
    m_lastSearchable = m_lastTool = m_files;

    doRestoreGeometry();

    connect(g_global, SIGNAL(actionSaveGeometry()), SLOT(handleSaveGeometry()));
    connect(g_global, SIGNAL(actionSaveOrder()), SLOT(handleSaveOrder()));
    connect(g_listener, SIGNAL(commandModeChanged(bool)), a_commandMode, SLOT(setChecked(bool)));
    connect(g_listener, SIGNAL(presModeChanged(bool)), SLOT(handlePresMode(bool)));
    connect(g_listener, SIGNAL(connectedChanged(bool)), SLOT(handleConnectedChanged(bool)));

    connect(m_manager, SIGNAL(titleChanged(const QString&)), SLOT(handleTitle(const QString&)));
    connect(m_manager, SIGNAL(finished()), SLOT(close()));

    connect(g_listener->taskmodel(), SIGNAL(taskAdded(TermTask*)),
            SLOT(handleTaskChange(TermTask*)));
    connect(g_listener->taskmodel(), SIGNAL(taskFinished(TermTask*)),
            SLOT(handleTaskChange(TermTask*)));

    a_commandMode->setCheckable(true);
    a_menuBarShown->setCheckable(true);
    a_statusBarShown->setCheckable(true);
    a_fullScreen->setCheckable(true);
    a_presMode->setCheckable(true);

    a_commandMode->setChecked(g_listener->commandMode());
    a_fullScreen->setChecked(isFullScreen());

    setMenuBarVisible(g_global->menuBar());
    setStatusBarVisible(g_global->statusBar());

    if (g_listener->presMode())
        handlePresMode(true);

    handleConnectedChanged(g_listener->connected());
    m_status->showMinor(TR_TEXT1.arg(FRIENDLY_NAME, PROJECT_VERSION));
}

MainWindow::~MainWindow()
{
    --s_nextIndex;
}

void
MainWindow::doRestoreGeometry()
{
    QByteArray gstate = g_state->fetch(WindowGeometryKey, m_index);
    QByteArray sstate = g_state->fetch(WindowStateKey, m_index);
    bool empty = gstate.isEmpty() && sstate.isEmpty();

    if (g_global->restoreGeometry() && !empty) {
        restoreGeometry(gstate);
        restoreState(sstate);
    } else {
        QRect geom = QApplication::desktop()->availableGeometry(this);
        int w = geom.width(), h = geom.height();

        if (w > h) {
            geom.setWidth(w / 2);
            w = m_index ? w / (1 << m_index) : 0;
            h = 0;
        } else {
            geom.setHeight(h / 2);
            w = 0;
            h = m_index ? h / (1 << m_index) : 0;
        }
        geom.translate(w, h);
        setGeometry(geom);

        QList<QDockWidget*> docks({ m_terminals });
        QList<int> sizes({ geom.height() * DOCK_SCALE });
        resizeDocks(docks, sizes, Qt::Vertical);
    }

    SplitLayoutReader split;
    if (g_global->restoreGeometry()) {
        split.parse(g_state->fetch(WindowLayoutKey, m_index));
    }
    m_widget->populate(split);
}

void
MainWindow::menuAction(const QString &slot)
{
    QString action(slot), keymap;
    TermShortcut result;
    bool found = m_manager->lookupShortcut(action, keymap, &result);

    if (!found) {
        // QString m(TR_TEXT2);
        // m_status->showMajor(m.arg(action, keymap));
    } else if (result.additional.isEmpty()) {
        QString m(TR_TEXT3);
        m_status->showMajor(m.arg(result.expression, action, keymap));
    } else {
        QString m(TR_TEXT3 + A(" (%4)"));
        m_status->showMajor(m.arg(result.expression, action, keymap, result.additional));
    }

    m_manager->invokeSlot(slot);
}

bool
MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_manager->setActive(true);
        g_listener->setManager(m_manager);
        m_filesWidget->setActive(true);
        break;
    case QEvent::WindowDeactivate:
        m_manager->setActive(false);
        m_filesWidget->setActive(false);
        break;
    case QEvent::Polish:
        QMainWindow::event(event);
        m_status->doPolish();
        static_cast<KeymapWidget*>(m_keymap->widget())->doPolish();
        return true;
    default:
        break;
    }

    return QMainWindow::event(event);
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    if (!g_listener->presMode()) {
        m_jobsWidget->saveState(m_index);
        m_notesWidget->saveState(m_index);
        m_tasksWidget->saveState(m_index);
        m_filesWidget->saveState(m_index);
        m_searchWidget->saveState(m_index);

        if (g_global->saveOrder())
            handleSaveOrder();
        if (g_global->saveGeometry())
            handleSaveGeometry();
    }
    event->accept();
}

void
MainWindow::handleSaveGeometry()
{
    SplitLayoutWriter split;
    m_widget->saveLayout(split);

    g_state->store(WindowGeometryKey, saveGeometry(), m_index);
    g_state->store(WindowStateKey, saveState(), m_index);
    g_state->store(WindowLayoutKey, split.result(), m_index);
}

void
MainWindow::handleSaveOrder()
{
    OrderLayoutWriter order;
    m_manager->saveOrder(order);

    g_state->store(WindowOrderKey, order.result(), m_index);
}

void
MainWindow::handleTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(APP_NAME);
        setWindowIconText(APP_NAME);
    } else {
        setWindowTitle(title);
        setWindowIconText(title);
    }
}

void
MainWindow::handleObjectDestroyed(QObject *obj)
{
    m_popups.remove(obj);
}

void
MainWindow::handleConnectedChanged(bool connected)
{
    if (connected) {
        m_status->clearStatus(STATUSPRIO_CONNECTION);
    } else {
        QString msg(L("<font style='font-weight:bold;background-color:%1;"
                      "color:%2'>%3</font> - %4"));

        msg = msg.arg(g_global->color(MajorBg).name(),
                      g_global->color(MajorFg).name(),
                      TR_TEXT5, TR_TEXT6);

        m_status->showStatus(msg, STATUSPRIO_CONNECTION);
    }
}

void
MainWindow::bringUp()
{
    QMainWindow::show();

    switch (g_listener->failureType()) {
    case 1:
        errBox(g_listener->failureTitle(), g_listener->failureMsg(), this)->show();
        g_listener->clearFailure();
        break;
#if USE_SYSTEMD
    case 2:
        showSystemdSetup(m_manager);
        g_listener->clearFailure();
        break;
#endif
    }
}

void
MainWindow::liftDockWidget(DockWidget *target)
{
    int idx = m_docklru.indexOf(target);
    switch (idx) {
    case 0:
        return;
    case -1:
        m_docklru.prepend(target);
        break;
    default:
        m_docklru.move(idx, 0);
    }

    if (target->istool()) {
        m_lastTool = target;

        if (target->searchable())
            m_lastSearchable = target;
    }
}

void
MainWindow::popDockWidget(DockWidget *target)
{
    target->raise();
}

void
MainWindow::unpopDockWidget(DockWidget *target, bool force)
{
    if (!target->visible())
        return;

    auto tabs = tabifiedDockWidgets(target);
    for (auto candidate: m_docklru) {
        if (candidate != target && tabs.contains(candidate)) {
            candidate->raise();
            return;
        }
    }

    if (force && !tabs.isEmpty())
        tabs.front()->raise();
}

bool
MainWindow::menuBarVisible() const
{
    return menuBar()->isVisible();
}

bool
MainWindow::statusBarVisible() const
{
    return statusBar()->isVisible();
}

void
MainWindow::setMenuBarVisible(bool visible)
{
    menuBar()->setVisible(visible);
    a_menuBarShown->setChecked(visible);
}

void
MainWindow::setStatusBarVisible(bool visible)
{
    statusBar()->setVisible(visible);
    a_statusBarShown->setChecked(visible);
}

void
MainWindow::setFullScreenMode(bool enabled)
{
    if (enabled)
        showFullScreen();
    else
        showNormal();

    a_fullScreen->setChecked(enabled);
}

void
MainWindow::handlePresMode(bool enabled)
{
    if (enabled) {
        if (g_global->presTools()) {
            m_epState = saveState();
            m_files->close();
            m_tasks->close();
            m_notes->close();
            m_jobs->close();
            m_search->close();
            m_commands->close();
            m_keymap->close();
            m_terminals->close();
        }
        if ((m_epFullScreen = !isFullScreen() && m_manager->primary() && g_global->presFullScreen()))
            setFullScreenMode(true);
        if ((m_epMenuBar = a_menuBarShown->isChecked() && g_global->presMenuBar()))
            setMenuBarVisible(false);
        if ((m_epStatusBar = a_statusBarShown->isChecked() && g_global->presStatusBar()))
            setStatusBarVisible(false);
    }
    else {
        if (isFullScreen() && m_epFullScreen)
            setFullScreenMode(false);
        if (m_epMenuBar)
            setMenuBarVisible(true);
        if (m_epStatusBar)
            setStatusBarVisible(true);
        if (!m_epState.isEmpty())
            restoreState(m_epState);
    }

    a_presMode->setChecked(enabled);
}

void
MainWindow::handleTaskChange(TermTask *task)
{
    if (m_timerId != 0) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
    if (g_global->raiseTasks())
    {
        if (!m_tasks->visible() && !task->questionCancel() && m_manager->primary()) {
            m_tasks->raise();
            m_tasks->setAutopopped(true);
        }

        if (m_tasks->autopopped() && !g_listener->taskmodel()->activeTask()) {
            m_timerId = startTimer(g_global->taskTime());
        }
    }
}

void
MainWindow::timerEvent(QTimerEvent *)
{
    killTimer(m_timerId);
    m_timerId = 0;

    if (m_tasks->autopopped()) {
        m_tasks->setAutopopped(false);
        unpopDockWidget(m_tasks);
    }
}

const QColor &
MainWindow::getGlobalColor(ColorName name) const
{
    return g_global->color(name);
}

void
MainWindow::setGlobalColor(ColorName name, const QColor &value)
{
    g_global->setColor(name, value);
}
