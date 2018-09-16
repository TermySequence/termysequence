// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "app/color.h"
#include "url.h"

#include <QMainWindow>
#include <QHash>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE
class TermManager;
class ServerInstance;
class TermInstance;
class MainWidget;
class MainStatus;
class DockWidget;
class SuggestWidget;
class SearchWidget;
class JobWidget;
class NoteWidget;
class TaskWidget;
class FileWidget;
class ThumbArrange;
class TermMarks;
class TermMark;
class TermModtimes;
class TermTask;
struct TermJob;
struct TermNote;
struct TermContentPopup;

#define COLORPROP(name, constant) \
    inline const QColor& name() const { return getGlobalColor(constant); } \
    inline void set ## constant(const QColor &val) { setGlobalColor(constant, val); }

class MainWindow final: public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QColor minorBg READ minorBg WRITE setMinorBg)
    Q_PROPERTY(QColor minorFg READ minorFg WRITE setMinorFg)
    Q_PROPERTY(QColor majorBg READ majorBg WRITE setMajorBg)
    Q_PROPERTY(QColor majorFg READ majorFg WRITE setMajorFg)
    Q_PROPERTY(QColor startFg READ startFg WRITE setStartFg)
    Q_PROPERTY(QColor errorFg READ errorFg WRITE setErrorFg)
    Q_PROPERTY(QColor finishFg READ finishFg WRITE setFinishFg)
    Q_PROPERTY(QColor cancelFg READ cancelFg WRITE setCancelFg)
    Q_PROPERTY(QColor connFg READ connFg WRITE setConnFg)
    Q_PROPERTY(QColor disconnFg READ disconnFg WRITE setDisconnFg)
    Q_PROPERTY(QColor bellFg READ bellFg WRITE setBellFg)

    friend class DynamicMenu;

private:
    TermManager *m_manager;
    MainWidget *m_widget;
    MainStatus *m_status;
    int m_index;
    int m_timerId = 0;

    bool m_epFullScreen = false;
    bool m_epMenuBar = false;
    bool m_epStatusBar = false;
    QByteArray m_epState;

    SuggestWidget *m_commandsWidget;
    SearchWidget *m_searchWidget;
    JobWidget *m_jobsWidget;
    NoteWidget *m_notesWidget;
    TaskWidget *m_tasksWidget;
    FileWidget *m_filesWidget;

    DockWidget *m_terminals, *m_keymap, *m_commands, *m_search;
    DockWidget *m_jobs, *m_notes, *m_tasks, *m_files;
    DockWidget *m_lastTool, *m_lastSearchable;

    QAction *a_commandMode;
    QAction *a_dockTerminals;
    QAction *a_dockKeymap;
    QAction *a_dockCommands;
    QAction *a_dockSearch;
    QAction *a_dockJobs;
    QAction *a_dockNotes;
    QAction *a_dockTasks;
    QAction *a_dockFiles;
    QAction *a_menuBarShown;
    QAction *a_statusBarShown;
    QAction *a_fullScreen;
    QAction *a_presMode;
    QAction *a_toolBarShown;
    QAction *a_toolHeaderShown;
    QAction *a_statusTipForwarder;

    void createMenus();
    void doRestoreGeometry();

    QList<DockWidget*> m_docklru;
    QHash<QObject*,QMenu*> m_popups;

private slots:
    void handleObjectDestroyed(QObject *term);
    void handleConnectedChanged(bool connected);
    void handlePresMode(bool enabled);
    void handleTitle(const QString &title);
    void handleTaskChange(TermTask *task);

protected:
    bool event(QEvent *event);
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

public slots:
    void handleSaveGeometry();
    void handleSaveOrder();

public:
    MainWindow(TermManager *manager);
    ~MainWindow();

    inline TermManager* manager() { return m_manager; }
    inline MainStatus* status() { return m_status; }

    inline SuggestWidget* commandsWidget() { return m_commandsWidget; }
    inline SearchWidget* searchWidget() { return m_searchWidget; }
    inline JobWidget* jobsWidget() { return m_jobsWidget; }
    inline NoteWidget* notesWidget() { return m_notesWidget; }
    inline TaskWidget* tasksWidget() { return m_tasksWidget; }
    inline FileWidget* filesWidget() { return m_filesWidget; }

    inline DockWidget* terminalsDock() { return m_terminals; }
    inline DockWidget* keymapDock() { return m_keymap; }
    inline DockWidget* commandsDock() { return m_commands; }
    inline DockWidget* searchDock() { return m_search; }
    inline DockWidget* jobsDock() { return m_jobs; }
    inline DockWidget* notesDock() { return m_notes; }
    inline DockWidget* tasksDock() { return m_tasks; }
    inline DockWidget* filesDock() { return m_files; }

    inline DockWidget* lastTool() { return m_lastTool; }
    inline DockWidget* lastSearchable() { return m_lastSearchable; }
    void liftDockWidget(DockWidget *target);
    void popDockWidget(DockWidget *target);
    void unpopDockWidget(DockWidget *target, bool force = false);

    QMenu* getTermPopup(TermInstance *term);
    QMenu* getThumbPopup(ThumbArrange *thumb);
    QMenu* getServerPopup(ServerInstance *server);
    QMenu* getMarksPopup(TermMarks *marks);
    QMenu* getJobPopup(TermInstance *term, regionid_t id, QWidget *parent);
    QMenu* getNotePopup(TermInstance *term, regionid_t id, QWidget *parent);
    QMenu* getModtimePopup(TermModtimes *modtimes);
    QMenu* getJobPopup(TermJob *job);
    QMenu* getNotePopup(TermNote *note);
    QMenu* getCommandPopup(int index);
    QMenu* getTaskPopup(QWidget *widget, TermTask *task, bool fileonly = false);
    QMenu* getImagePopup(TermInstance *term, const TermContentPopup *params);
    QMenu* getLinkPopup(TermInstance *term, QString url);
    QMenu* getFilePopup(ServerInstance *server, const TermUrl &tu);
    QMenu* getCustomPopup(TermInstance *term, const QString &spec, const QString &icon);
    QMenu* getManagePopup(TermInstance *term, QWidget *parent);
    QMenu* getManagePopup(ServerInstance *term, QWidget *parent);

    void bringUp();
    void menuAction(const QString &slot);

    bool menuBarVisible() const;
    bool statusBarVisible() const;
    void setMenuBarVisible(bool visible);
    void setStatusBarVisible(bool visible);
    void setFullScreenMode(bool enabled);

public:
    // Colors
    const QColor& getGlobalColor(ColorName name) const;
    void setGlobalColor(ColorName name, const QColor &value);

    COLORPROP(minorBg, MinorBg)
    COLORPROP(minorFg, MinorFg)
    COLORPROP(majorBg, MajorBg)
    COLORPROP(majorFg, MajorFg)
    COLORPROP(startFg, StartFg)
    COLORPROP(errorFg, ErrorFg)
    COLORPROP(finishFg, FinishFg)
    COLORPROP(cancelFg, CancelFg)
    COLORPROP(connFg, ConnFg)
    COLORPROP(disconnFg, DisconnFg)
    COLORPROP(bellFg, BellFg)
};
