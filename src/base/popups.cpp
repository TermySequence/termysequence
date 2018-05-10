// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "app/attr.h"
#include "app/icons.h"
#include "mainwindow.h"
#include "menubase.h"
#include "menumacros.h"
#include "server.h"
#include "term.h"
#include "thumbarrange.h"
#include "marks.h"
#include "mark.h"
#include "modtimes.h"
#include "job.h"
#include "note.h"
#include "task.h"
#include "contentmodel.h"
#include "settings/settings.h"
#include "settings/global.h"

QMenu *
MainWindow::getCustomPopup(TermInstance *term, const QString &spec, const QString &icon)
{
    const QStringList list = spec.split('\0');
    QMenu *inheritMenu = nullptr;
    QString inheritText;
    QAction *a = nullptr;
    bool added = false;

    POPUP_DOH("pmenu-custom", this);

    for (int i = 0, n = list.size(); i + 5 <= n; i += 5)
    {
        auto s1 = list[i + 1], s2 = list[i + 2], s3 = list[i + 3], s4 = list[i + 4];
        switch (list[i].toUInt()) {
        case 0:
            // Separator
            MS();
            break;
        case 1:
            // Menu Item
            a = MI(s2, s4, ThumbIcon::fromTheme(s3.toStdString()), s1);
            added = true;
            continue;
        case 2:
            // Confirmation for previous menu item
            if (a)
                menu->setConfirmation(a, s1);
            break;
        case 5:
            // Inherit from File menu, must be first in list
            if (i == 0) {
                TermUrl tu = TermUrl::parse(s1);
                if (tu.isLocalFile()) {
                    inheritMenu = getFilePopup(term->server(), tu);
                    inheritText = s2;
                }
            }
            break;
        default:
            // TODO: 3: Start submenu
            // TODO: 4: Return from submenu
            break;
        }

        a = nullptr;
    }

    QMenu *termMenu = getTermPopup(term);

    if (!added) {
        menu->deleteLater();
        if (!(menu = static_cast<DynamicMenu*>(inheritMenu)))
            return termMenu;
    }
    else if (inheritMenu) {
        // adjust menu DOH to inheritMenu
        disconnect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
        connect(inheritMenu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));

        a = inheritMenu->insertSeparator(inheritMenu->actions().front());
        a = inheritMenu->insertMenu(a, menu);
        a->setIcon(ThumbIcon::getIcon(ThumbIcon::SemanticType, icon));
        a->setText(inheritText);
        menu = static_cast<DynamicMenu*>(inheritMenu);
    }

    a = menu->insertSeparator(menu->actions().front());
    a = menu->insertMenu(a, termMenu);
    a->setIcon(QI(ICON_MENU_TERMINAL));
    a->setText(TL("pmenu-custom", "Terminal Menu"));
    return menu;
}

QMenu *
MainWindow::getLinkPopup(TermInstance *term, QString urlStr)
{
    QUrl url(urlStr);
    urlStr = url.toString(QUrl::FullyEncoded);
    QString lcombo = term->server()->idStr() + '|' + urlStr;

    POPUP_DOH("pmenu-link", this);
    menu->addMenu(getTermPopup(term), TN("pmenu-link", "&Terminal Menu"), ICON_MENU_TERMINAL);
    MS();

    MI(TN("pmenu-link", "Open &Link"), ACT_OPEN_URL, ICON_OPEN_URL, L("OpenDesktopUrl|") + urlStr);
    const auto elt = g_settings->defaultLauncher();
    MI(TL("pmenu-link", "&Open with %1").arg(elt.first), ACT_OPEN_URL_ARG.arg(elt.first), elt.second, L("OpenUrl||") + lcombo);

    DynamicMenu *subMenu;
    SUBMENU("pmenu-link-open", TN("pmenu-link", "Op&en with"), ICON_OPEN_WITH);
    for (const auto &elt: g_settings->getLaunchers(url))
        SI(TL("pmenu-link-open", "Open with %1").arg(elt.first), ACT_OPEN_URL_ARG.arg(elt.first), elt.second, L("OpenUrl|%1|%2").arg(elt.first, lcombo));
    SI(TN("pmenu-link-open", "Op&en with") "...", ACT_OPEN_URL_CHOOSE, ICON_CHOOSE_ITEM, L("OpenUrl|%1|%2").arg(g_str_PROMPT_PROFILE, lcombo));

    MS();
    MI(TN("pmenu-link", "&Copy Link"), ACT_COPY_URL, ICON_COPY_URL, L("CopyUrl|") + urlStr);
    return menu;
}

QMenu *
MainWindow::getImagePopup(TermInstance *term, const TermContentPopup *params)
{
    QString combo = term->idStr() + '|' + params->id;
    QMenu *subMenu;
    const char *text;

    POPUP_DOH("pmenu-image", this);
    if (params->addTermMenu) {
        menu->addMenu(getTermPopup(term), TN("pmenu-image", "&Terminal Menu"), ICON_MENU_TERMINAL);
        MS();
    }
    if (params->tu.isLocalFile()) {
        subMenu = getFilePopup(term->server(), params->tu);
        // adjust subMenu DOH to us
        disconnect(subMenu, SIGNAL(aboutToHide()), subMenu, SLOT(deleteLater()));
        connect(menu, SIGNAL(aboutToHide()), subMenu, SLOT(deleteLater()));
        menu->addMenu(subMenu, TN("pmenu-image", "Original &File"));
    } else {
        MI(TN("pmenu-image", "No Original File"), g_mtstr, QIcon(), L("ViewTerminalContent"))->setEnabled(false);
    }
    MS();
    if (!params->isshown) {
        MI(TN("pmenu-image", "&Show Thumbnail"), ACT_FETCH_IMAGE, ICON_FETCH_IMAGE, L("FetchImage|") + combo);
    }
    text = params->isimage ? TN("pmenu-image", "Download &Thumbnail") : TN("pmenu-image", "Download &Content");
    MI(text, ACT_DOWNLOAD_IMAGE, ICON_DOWNLOAD_IMAGE, L("DownloadImage|") + combo);
    text = params->isimage ? TN("pmenu-image", "&Copy Thumbnail") : TN("pmenu-image", "&Copy Content");
    MI(text, ACT_COPY_IMAGE, ICON_COPY_IMAGE, L("CopyImage||") + combo);
    MS();
    if (params->addTermMenu) {
        MI(TN("pmenu-image", "&View Inline Content"), ACT_VIEW_TERMINAL_CONTENT, ICON_VIEW_TERMINAL_CONTENT, L("ViewTerminalContent|") + params->id);
    } else {
        MI(TN("pmenu-image", "&View in Terminal"), ACT_SCROLL_IMAGE, ICON_SCROLL_IMAGE, L("ScrollImage|") + combo);
    }
    return menu;
}

QMenu *
MainWindow::getFilePopup(ServerInstance *server, const TermUrl &tu)
{
    bool d = tu.hasDir();
    bool f = tu.hasFile();
    bool i = f && !tu.fileIsDir();
    bool r = server && !server->local();
    bool s = r || g_global->localDownload();

    QString serverIdStr = server ? server->idStr() : g_mtstr;
    QString fcombo = serverIdStr + '|' + tu.hostPath();
    QString dcombo = serverIdStr + '|' + tu.hostDir();
    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-file", this);

    const auto elt = g_settings->defaultLauncher();
    MI(TL("pmenu-file", "&Open with %1").arg(elt.first), ACT_OPEN_FILE, elt.second, L("OpenFile||") + fcombo, d);
    if (!d)
        goto skip;

    SUBMENU("pmenu-file-open", TN("pmenu-file", "Op&en with"), ICON_OPEN_WITH);
    for (const auto &elt: g_settings->getLaunchers(tu))
        SI(TL("pmenu-file-open", "Open with %1").arg(elt.first), ACT_OPEN_FILE_ARG.arg(elt.first), elt.second, L("OpenFile|%1|%2").arg(elt.first, fcombo));
    SI(TN("pmenu-file-open", "Op&en with") "...", ACT_OPEN_FILE_CHOOSE, ICON_CHOOSE_ITEM, L("OpenFile|%1|%2").arg(g_str_PROMPT_PROFILE, fcombo));
    SS();
    SI(TN("pmenu-file-open", "Mount &Read-Only"), ACT_MOUNT_FILE_RO, ICON_MOUNT_FILE_RO, L("MountFile|1|") + fcombo, r);
    SI(TN("pmenu-file-open", "Mount Read-&Write"), ACT_MOUNT_FILE_RW, ICON_MOUNT_FILE_RW, L("MountFile|0|") + fcombo, r);
    if (i) {
        SS();
        SI(TN("pmenu-file-open", "&Copy Contents as Text"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyFile|0|") + fcombo);
        SI(TN("pmenu-file-open", "Copy Contents as &Image"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyFile|1|") + fcombo);

        MI(TN("pmenu-file", "&Download File"), ACT_DOWNLOAD_FILE, ICON_DOWNLOAD_FILE, L("DownloadFile|") + fcombo, s);
    }
skip:
    MI(TN("pmenu-file", "Upload to &Here"), ACT_UPLOAD_FILE, ICON_UPLOAD_FILE, L("UploadFile|") + fcombo, s && d);
    if (f) {
        MI(TN("pmenu-file", "&Upload to Parent Folder"), ACT_UPLOAD_DIR, ICON_UPLOAD_DIR, L("UploadToDirectory|") + dcombo, s);
        MS();
        MI(TN("pmenu-file", "&Rename File"), ACT_RENAME_FILE, ICON_RENAME_FILE, L("RenameFile|") + fcombo);
        MI(TN("pmenu-file", "&Delete File"), ACT_DELETE_FILE, ICON_DELETE_FILE, L("DeleteFile|") + fcombo);
    }
    MS();
    MI(TN("pmenu-file", "&Write Path"), ACT_WRITE_FILE_PATH, ICON_WRITE_FILE_PATH, L("WriteFilePath|") + tu.path(), d);
    if (f) {
        MI(TN("pmenu-file", "Write &Parent Path"), ACT_WRITE_DIRECTORY_PATH, ICON_WRITE_DIRECTORY_PATH, L("WriteDirectoryPath|") + tu.dir());
    }
    MI(TN("pmenu-file", "&Copy Path"), ACT_COPY_FILE_PATH, ICON_COPY_FILE_PATH, L("CopyFilePath|") + tu.path(), d);
    if (f) {
        MI(TN("pmenu-file", "Copy P&arent Path"), ACT_COPY_DIRECTORY_PATH, ICON_COPY_DIRECTORY_PATH, L("CopyDirectoryPath|") + tu.dir());
    }
    return menu;
}

QMenu *
MainWindow::getCommandPopup(int index)
{
    bool e = index >= 0;
    auto *menu = new DynamicMenu("pmenu-command", this, this);

    // caller is responsible for freeing menu

    MI(TN("pmenu-command", "&Write Command"), ACT_WRITE_COMMAND, ICON_WRITE_COMMAND, L("WriteSuggestion|%1").arg(index), e);
    MI(TN("pmenu-command", "Write Command with &Newline"), ACT_WRITE_COMMAND_NEWLINE, ICON_WRITE_COMMAND_NEWLINE, L("WriteSuggestionNewline|%1").arg(index), e);
    MI(TN("pmenu-command", "&Copy Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopySuggestion|%1").arg(index), e);
    MS();
    MI(TN("pmenu-command", "&Remove Command"), ACT_REMOVE_COMMAND, ICON_REMOVE_COMMAND, L("RemoveSuggestion|%1").arg(index), e);
    return menu;
}

QMenu *
MainWindow::getTaskPopup(QWidget *widget, TermTask *task, bool fileonly)
{
    QString taskIdStr, termIdStr, serverIdStr, outfile;
    bool e = task, t, r, c;
    if (e) {
        taskIdStr = task->idStr();
        termIdStr = QString::fromLatin1(task->termId().str().c_str());
        serverIdStr = QString::fromLatin1(task->serverId().str().c_str());
        outfile = task->launchfile();
    }
    t = e && task->term();
    r = e && !task->finished();
    c = e && task->clonable();

    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-task", widget);

    if (!outfile.isEmpty()) {
        const auto elt = g_settings->defaultLauncher();
        MI(TL("pmenu-task", "&Open with %1").arg(elt.first), ACT_OPEN_FILE, elt.second, L("OpenTaskFile||") + taskIdStr);

        SUBMENU("pmenu-task-open", TN("pmenu-task", "Op&en with"), ICON_OPEN_WITH);
        for (const auto &elt: g_settings->getLaunchers(QUrl::fromLocalFile(outfile)))
            SI(TL("pmenu-task-open", "Open with %1").arg(elt.first), ACT_OPEN_FILE_ARG.arg(elt.first), elt.second, L("OpenTaskFile|%1|%2").arg(elt.first, taskIdStr));
        SI(TN("pmenu-task-open", "Op&en with") "...", ACT_OPEN_FILE_CHOOSE, ICON_CHOOSE_ITEM, L("OpenTaskFile|%1|%2").arg(g_str_PROMPT_PROFILE, taskIdStr));

        MI(TL("pmenu-task", "Open &Folder"), ACT_OPEN_TASK_DIRECTORY, ICON_OPEN_DIRECTORY, L("OpenTaskDirectory||") + taskIdStr);
        MI(TL("pmenu-task", "Open &Terminal"), ACT_OPEN_TASK_DIRECTORY, ICON_OPEN_TERMINAL, L("OpenTaskTerminal||") + taskIdStr);
        MS();
        MI(TN("pmenu-task", "&Write Path"), ACT_WRITE_FILE_PATH, ICON_WRITE_FILE_PATH, L("WriteTaskFilePath|") + taskIdStr);
        MI(TN("pmenu-task", "Write &Parent Path"), ACT_WRITE_DIRECTORY_PATH, ICON_WRITE_DIRECTORY_PATH, L("WriteTaskDirectoryPath|") + taskIdStr);
        MI(TN("pmenu-task", "&Copy Path"), ACT_COPY_FILE_PATH, ICON_COPY_FILE_PATH, L("CopyTaskFilePath|") + taskIdStr);
        MI(TN("pmenu-task", "Copy P&arent Path"), ACT_COPY_DIRECTORY_PATH, ICON_COPY_DIRECTORY_PATH, L("CopyTaskDirectoryPath|") + taskIdStr);
        MS();
    }
    if (fileonly)
        goto out;

    MI(TN("pmenu-task", "Show &Status"), ACT_INSPECT_TASK, ICON_INSPECT_ITEM, L("InspectTask|") + taskIdStr, e);
    MI(TN("pmenu-task", "&Cancel Task"), ACT_CANCEL_TASK, ICON_CANCEL_TASK, L("CancelTask|") + taskIdStr, r);
    MI(TN("pmenu-task", "&Re-run Task"), ACT_RESTART_TASK, ICON_RESTART_TASK, L("RestartTask|") + taskIdStr, c);
    MS();

    SUBMENU("pmenu-task-filter", TN("pmenu-task", "&Filter"), ICON_FILTER);
    SI(TN("pmenu-task-filter", "Exclude this Terminal"), ACT_TOOL_FILTER_EXCLUDE_TERMINAL, ICON_TOOL_FILTER_EXCLUDE_TERMINAL, L("ToolFilterExcludeTerminal|") + termIdStr, t);
    SI(TN("pmenu-task-filter", "Exclude this Server"), ACT_TOOL_FILTER_EXCLUDE_SERVER, ICON_TOOL_FILTER_EXCLUDE_SERVER, L("ToolFilterExcludeServer|") + serverIdStr);
    SS();
    SI(TN("pmenu-task-filter", "Include Only this Terminal"), ACT_TOOL_FILTER_SET_TERMINAL, ICON_TOOL_FILTER_SET_TERMINAL, L("ToolFilterSetTerminal|") + termIdStr, t);
    SI(TN("pmenu-task-filter", "Include Only this Server"), ACT_TOOL_FILTER_SET_SERVER, ICON_TOOL_FILTER_SET_SERVER, L("ToolFilterSetServer|") + serverIdStr);
    SI(TN("pmenu-task-filter", "Include the active terminal"), ACT_TOOL_FILTER_ADD_TERMINAL, ICON_TOOL_FILTER_ADD_TERMINAL, L("ToolFilterAddTerminal"));
    SI(TN("pmenu-task-filter", "Include the active server"), ACT_TOOL_FILTER_ADD_SERVER, ICON_TOOL_FILTER_ADD_SERVER, L("ToolFilterAddServer"));
    SS();
    SI(TN("pmenu-task-filter", "Include nothing"), ACT_TOOL_FILTER_INCLUDE_NOTHING, ICON_TOOL_FILTER_INCLUDE_NOTHING, L("ToolFilterIncludeNothing"));
    SI(TN("pmenu-task-filter", "Reset Filter"), ACT_TOOL_FILTER_RESET, ICON_TOOL_FILTER_RESET, L("ToolFilterReset"));

    MI(TN("pmenu-task", "Remove Completed Tasks"), ACT_REMOVE_TASKS, ICON_TASK_VIEW_REMOVE_TASKS, L("RemoveTasks"));
out:
    return menu;
}

QMenu *
MainWindow::getJobPopup(TermJob *job)
{
    QString termIdStr, serverIdStr, regionIdStr;
    bool e = job;
    if (e) {
        termIdStr = QString::fromLatin1(job->termId.str().c_str());
        serverIdStr = QString::fromLatin1(job->serverId.str().c_str());
        regionIdStr = QString::number(job->region);
    }

    QString combo = regionIdStr + '|' + termIdStr;
    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-job", this);
    MI(TN("pmenu-job", "&Write Command"), ACT_WRITE_COMMAND, ICON_WRITE_COMMAND, L("WriteCommand|") + combo);
    MI(TN("pmenu-job", "Write Command with &Newline"), ACT_WRITE_COMMAND_NEWLINE, ICON_WRITE_COMMAND_NEWLINE, L("WriteCommandNewline|") + combo);
    MI(TN("pmenu-job", "&Copy Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopyCommand|") + combo);
    MS();

    SUBMENU("pmenu-job-filter", TN("pmenu-job", "&Filter"), ICON_FILTER);
    SI(TN("pmenu-job-filter", "Exclude this Terminal"), ACT_TOOL_FILTER_EXCLUDE_TERMINAL, ICON_TOOL_FILTER_EXCLUDE_TERMINAL, L("ToolFilterExcludeTerminal|") + termIdStr);
    SI(TN("pmenu-job-filter", "Exclude this Server"), ACT_TOOL_FILTER_EXCLUDE_SERVER, ICON_TOOL_FILTER_EXCLUDE_SERVER, L("ToolFilterExcludeServer|") + serverIdStr);
    SS();
    SI(TN("pmenu-job-filter", "Include Only this Terminal"), ACT_TOOL_FILTER_SET_TERMINAL, ICON_TOOL_FILTER_SET_TERMINAL, L("ToolFilterSetTerminal|") + termIdStr);
    SI(TN("pmenu-job-filter", "Include Only this Server"), ACT_TOOL_FILTER_SET_SERVER, ICON_TOOL_FILTER_SET_SERVER, L("ToolFilterSetServer|") + serverIdStr);
    SI(TN("pmenu-job-filter", "Include the active terminal"), ACT_TOOL_FILTER_ADD_TERMINAL, ICON_TOOL_FILTER_ADD_TERMINAL, L("ToolFilterAddTerminal"));
    SI(TN("pmenu-job-filter", "Include the active server"), ACT_TOOL_FILTER_ADD_SERVER, ICON_TOOL_FILTER_ADD_SERVER, L("ToolFilterAddServer"));
    SS();
    SI(TN("pmenu-job-filter", "Include nothing"), ACT_TOOL_FILTER_INCLUDE_NOTHING, ICON_TOOL_FILTER_INCLUDE_NOTHING, L("ToolFilterIncludeNothing"));
    SI(TN("pmenu-job-filter", "Reset Filter"), ACT_TOOL_FILTER_RESET, ICON_TOOL_FILTER_RESET, L("ToolFilterReset"));

    MS();
    MI(TN("pmenu-job", "&Jump to Start"), ACT_SCROLL_JOB_START, ICON_SCROLL_REGION_START, L("ScrollRegionStart|") + combo, e);
    MI(TN("pmenu-job", "Jump to &End"), ACT_SCROLL_JOB_END, ICON_SCROLL_REGION_END, L("ScrollRegionEnd|") + combo, e);
    MS();
    MI(TN("pmenu-job", "Copy &Output"), ACT_COPY_OUTPUT, ICON_COPY_OUTPUT, L("CopyOutput|") + combo, e);
    MI(TN("pmenu-job", "Copy &Job"), ACT_COPY_JOB, ICON_COPY_JOB, L("CopyJob|") + combo, e);
    MS();

    SUBMENU("pmenu-job-select", TN("pmenu-job", "&Select"), ICON_SELECT);
    SI(TN("pmenu-job-select", "&Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutput|") + combo, e);
    SI(TN("pmenu-job-select", "&Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommand|") + combo, e);
    SI(TN("pmenu-job-select", "&Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJob|") + combo, e);

    MS();
    MI(TN("pmenu-job", "&Annotate Job"), ACT_ANNOTATE_JOB, ICON_ANNOTATE_LINE, L("AnnotateRegion|") + combo, e);

    SUBMENU("pmenu-job-annotate", TN("pmenu-job", "Annotate"), ICON_ANNOTATE);
    SI(TN("pmenu-job-annotate", "&Command"), ACT_ANNOTATE_COMMAND, ICON_ANNOTATE_COMMAND, L("AnnotateCommand|") + combo, e);
    SI(TN("pmenu-job-annotate", "&Output"), ACT_ANNOTATE_OUTPUT, ICON_ANNOTATE_OUTPUT, L("AnnotateOutput|") + combo, e);

    MS();
    MI(TN("pmenu-job", "Remove Closed Terminals"), ACT_TOOL_FILTER_REMOVE_CLOSED, ICON_TOOL_FILTER_REMOVE_CLOSED, L("ToolFilterRemoveClosed"));
    return menu;
}

QMenu *
MainWindow::getNotePopup(TermNote *note)
{
    QString termIdStr, serverIdStr, regionIdStr;
    bool e = note;
    if (e) {
        termIdStr = QString::fromLatin1(note->termId.str().c_str());
        serverIdStr = QString::fromLatin1(note->serverId.str().c_str());
        regionIdStr = QString::number(note->region);
    }

    QString combo = regionIdStr + '|' + termIdStr;
    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-note", this);

    SUBMENU("pmenu-note-filter", TN("pmenu-note", "&Filter"), ICON_FILTER);
    SI(TN("pmenu-note-filter", "Exclude this Terminal"), ACT_TOOL_FILTER_EXCLUDE_TERMINAL, ICON_TOOL_FILTER_EXCLUDE_TERMINAL, L("ToolFilterExcludeTerminal|") + termIdStr);
    SI(TN("pmenu-note-filter", "Exclude this Server"), ACT_TOOL_FILTER_EXCLUDE_SERVER, ICON_TOOL_FILTER_EXCLUDE_SERVER, L("ToolFilterExcludeServer|") + serverIdStr);
    SS();
    SI(TN("pmenu-note-filter", "Include Only this Terminal"), ACT_TOOL_FILTER_SET_TERMINAL, ICON_TOOL_FILTER_SET_TERMINAL, L("ToolFilterSetTerminal|") + termIdStr);
    SI(TN("pmenu-note-filter", "Include Only this Server"), ACT_TOOL_FILTER_SET_SERVER, ICON_TOOL_FILTER_SET_SERVER, L("ToolFilterSetServer|") + serverIdStr);
    SI(TN("pmenu-note-filter", "Include the active terminal"), ACT_TOOL_FILTER_ADD_TERMINAL, ICON_TOOL_FILTER_ADD_TERMINAL, L("ToolFilterAddTerminal"));
    SI(TN("pmenu-note-filter", "Include the active server"), ACT_TOOL_FILTER_ADD_SERVER, ICON_TOOL_FILTER_ADD_SERVER, L("ToolFilterAddServer"));
    SS();
    SI(TN("pmenu-note-filter", "Include nothing"), ACT_TOOL_FILTER_INCLUDE_NOTHING, ICON_TOOL_FILTER_INCLUDE_NOTHING, L("ToolFilterIncludeNothing"));
    SI(TN("pmenu-note-filter", "Reset Filter"), ACT_TOOL_FILTER_RESET, ICON_TOOL_FILTER_RESET, L("ToolFilterReset"));

    MS();
    MI(TN("pmenu-note", "&Jump to Start"), ACT_SCROLL_NOTE_START, ICON_SCROLL_REGION_START, L("ScrollRegionStart|") + combo, e);
    MI(TN("pmenu-note", "Jump to &End"), ACT_SCROLL_NOTE_END, ICON_SCROLL_REGION_END, L("ScrollRegionEnd|") + combo, e);
    MS();
    MI(TN("pmenu-note", "&Remove Annotation"), ACT_REMOVE_NOTE, ICON_REMOVE_ANNOTATION, L("RemoveNote|") + combo, e);
    MI(TN("pmenu-note", "&Copy Annotated Text"), ACT_COPY_NOTE, ICON_COPY_NOTE, L("CopyRegion|") + combo, e);
    MI(TN("pmenu-note", "&Select Annotation"), ACT_SELECT_NOTE, ICON_SELECT_JOB, L("SelectJob|") + combo, e);
    MS();
    MI(TN("pmenu-note", "Remove Closed Terminals"), ACT_TOOL_FILTER_REMOVE_CLOSED, ICON_TOOL_FILTER_REMOVE_CLOSED, L("ToolFilterRemoveClosed"));
    return menu;
}

QMenu *
MainWindow::getModtimePopup(TermModtimes *modtimes)
{
    auto i = m_popups.constFind(modtimes);
    if (i != m_popups.cend())
        return *i;

    POPUP_OBJ("pmenu-modtimes", modtimes);
    MI(TN("pmenu-modtimes", "Set &Origin Here"), ACT_TIMING_SET_ORIGIN_CONTEXT, ICON_TIMING_SET_ORIGIN, L("TimingSetOriginContext"));
    MI(TN("pmenu-modtimes", "&Float Origin"), ACT_TIMING_FLOAT_ORIGIN, ICON_TIMING_FLOAT_ORIGIN, L("TimingFloatOrigin"));
    MS();
    const QString url = g_global->docUrl(A("widgets.html#timing"));
    MI(TN("pmenu-modtimes", "Timing &Help"), ACT_HELP_CONTENTS, ICON_HELP_CONTENTS, L("OpenDesktopUrl|") + url);
    return menu;
}

QMenu *
MainWindow::getNotePopup(TermInstance *term, regionid_t id, QWidget *parent)
{
    QString combo = QString::number(id) + '|' + term->idStr();
    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-nmark", parent);

    MI(TN("pmenu-nmark", "&Remove Annotation"), ACT_REMOVE_NOTE, ICON_REMOVE_ANNOTATION, L("RemoveNote|") + combo);
    MS();
    MI(TN("pmenu-nmark", "&Copy Annotated Text"), ACT_COPY_NOTE, ICON_COPY_JOB, L("CopyRegion|") + combo);

    SUBMENU("pmenu-nmark-copy", TN("pmenu-job", "Copy"), ICON_COPY);
    SI(TN("pmenu-nmark-copy", "Copy &Output"), ACT_COPY_OUTPUT, ICON_COPY_OUTPUT, L("CopyOutputContext"));
    SI(TN("pmenu-nmark-copy", "Copy &Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopyCommandContext"));
    SI(TN("pmenu-nmark-copy", "Copy &Job"), ACT_COPY_JOB, ICON_COPY_JOB, L("CopyJobContext"));

    MS();
    MI(TN("pmenu-nmark", "&Select Annotation"), ACT_SELECT_NOTE, ICON_SELECT_JOB, L("SelectJob|") + combo);

    SUBMENU("pmenu-nmark-select", TN("pmenu-job", "Select"), ICON_SELECT);
    SI(TN("pmenu-nmark-select", "Select &Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutputContext"));
    SI(TN("pmenu-nmark-select", "Select &Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommandContext"));
    SI(TN("pmenu-nmark-select", "Select &Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJobContext"));
    return menu;
}

QMenu *
MainWindow::getJobPopup(TermInstance *term, regionid_t id, QWidget *parent)
{
    QString combo = QString::number(id) + '|' + term->idStr();
    DynamicMenu *subMenu;

    POPUP_DOH("pmenu-jmark", parent);

    MI(TN("pmenu-jmark", "&Write Command"), ACT_WRITE_COMMAND, ICON_WRITE_COMMAND, L("WriteCommand|") + combo);
    MI(TN("pmenu-jmark", "Write Command &with Newline"), ACT_WRITE_COMMAND_NEWLINE, ICON_WRITE_COMMAND_NEWLINE, L("WriteCommandNewline|") + combo);
    MI(TN("pmenu-jmark", "&Copy Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopyCommand|") + combo);
    MS();
    MI(TN("pmenu-jmark", "&Jump to Start"), ACT_SCROLL_JOB_START, ICON_SCROLL_REGION_START, L("ScrollRegionStart|") + combo);
    MI(TN("pmenu-jmark", "Jump to &End"), ACT_SCROLL_JOB_END, ICON_SCROLL_REGION_END, L("ScrollRegionEnd|") + combo);
    MS();
    MI(TN("pmenu-jmark", "Copy &Output"), ACT_COPY_OUTPUT, ICON_COPY_OUTPUT, L("CopyOutput|") + combo);
    MI(TN("pmenu-jmark", "Copy &Job"), ACT_COPY_JOB, ICON_COPY_JOB, L("CopyJob|") + combo);
    MS();

    SUBMENU("pmenu-jmark-select", TN("pmenu-job", "&Select"), ICON_SELECT);
    SI(TN("pmenu-jmark-select", "&Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutput|") + combo);
    SI(TN("pmenu-jmark-select", "&Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommand|") + combo);
    SI(TN("pmenu-jmark-select", "&Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJob|") + combo);

    MS();
    MI(TN("pmenu-jmark", "&Annotate Job"), ACT_ANNOTATE_JOB, ICON_ANNOTATE_LINE, L("AnnotateRegion|") + combo);

    SUBMENU("pmenu-jmark-annotate", TN("pmenu-jmark", "Annotate"), ICON_ANNOTATE);
    SI(TN("pmenu-jmark-annotate", "&Command"), ACT_ANNOTATE_COMMAND, ICON_ANNOTATE_COMMAND, L("AnnotateCommand|") + combo);
    SI(TN("pmenu-jmark-annotate", "&Output"), ACT_ANNOTATE_OUTPUT, ICON_ANNOTATE_OUTPUT, L("AnnotateOutput|") + combo);
    return menu;
}

QMenu *
MainWindow::getMarksPopup(TermMarks *marks)
{
    auto i = m_popups.constFind(marks);
    if (i != m_popups.cend())
        return *i;

    POPUP_OBJ("pmenu-marks", marks);
    MI(TN("pmenu-marks", "&Jump to Start"), ACT_SCROLL_JOB_START, ICON_SCROLL_REGION_START, L("ScrollJobStartContext"));
    MI(TN("pmenu-marks", "Jump to &End"), ACT_SCROLL_JOB_END, ICON_SCROLL_REGION_END, L("ScrollJobEndContext"));
    MS();
    MI(TN("pmenu-marks", "Select &Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutputContext"));
    MI(TN("pmenu-marks", "Select &Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommandContext"));
    MI(TN("pmenu-marks", "Select &Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJobContext"));
    MS();
    MI(TN("pmenu-marks", "Annotate Line"), ACT_ANNOTATE_LINE_CONTEXT, ICON_ANNOTATE_LINE, L("AnnotateLineContext"));
    MI(TN("pmenu-marks", "Annotate Command"), ACT_ANNOTATE_COMMAND, ICON_ANNOTATE_COMMAND, L("AnnotateCommandContext"));
    MI(TN("pmenu-marks", "Annotate Output"), ACT_ANNOTATE_OUTPUT, ICON_ANNOTATE_OUTPUT, L("AnnotateOutputContext"));
    return menu;
}

QMenu *
MainWindow::getTermPopup(TermInstance *term)
{
    auto i = m_popups.constFind(term);
    if (i != m_popups.cend())
        return *i;

    const QString &termIdStr = term->idStr();

    POPUP_TOBJ(term, "pmenu-terminal", term);
    menu->enable(SettingsMenu);
    menu->enable(TermMenu);

    MI(TN("pmenu-terminal", "&Copy"), ACT_COPY, ICON_COPY, L("Copy|") + termIdStr);
    MI(TN("pmenu-terminal", "&Paste"), ACT_PASTE, ICON_PASTE, L("Paste|") + termIdStr);
    MI(TN("pmenu-terminal", "A&nnotate"), ACT_ANNOTATE_SELECTION, ICON_ANNOTATE_SELECTION, L("AnnotateSelection"));
    MS();
    MI(TN("pmenu-terminal", "Clone &Terminal"), ACT_CLONE_TERMINAL, ICON_CLONE_TERMINAL, L("CloneTerminal||") + termIdStr);
    MI(TN("pmenu-terminal", "Duplicate Terminal"), ACT_DUPLICATE_TERMINAL, ICON_DUPLICATE_TERMINAL, L("CloneTerminal|1|") + termIdStr);
    MS();
    MI(TN("pmenu-terminal", "Restore &Menu Bar"), ACT_SHOW_MENU_BAR, ICON_SHOW_MENU_BAR, L("ShowMenuBar"), DynRestoreMenuBar);
    MI(TN("pmenu-terminal", "Exit Presentation &Mode"), ACT_EXIT_PRESENTATION_MODE, ICON_EXIT_PRES_MODE, L("ExitPresentationMode"), DynExitPresentationMode);
    MI(TN("pmenu-terminal", "E&xit Full Screen"), ACT_EXIT_FULL_SCREEN, ICON_EXIT_FULL_SCREEN, L("ExitFullScreen"), DynExitFullScreenMode);
    MI(TN("pmenu-terminal", "&Input Follower"), ACT_INPUT_TOGGLE_FOLLOWER, ICON_INPUT_TOGGLE_FOLLOWER, L("InputToggleFollower"), DynInputFollower);
    MI(TN("pmenu-terminal", "&Stop Input Multiplexing"), ACT_INPUT_UNSET_LEADER, ICON_INPUT_UNSET_LEADER, L("InputUnsetLeader"), DynInputMultiplexing);
    MS();
    MI(TN("pmenu-terminal", "Adjust &Font") "...", ACT_ADJUST_TERMINAL_FONT, ICON_CHOOSE_FONT, L("AdjustTerminalFont|") + termIdStr);
    MI(TN("pmenu-terminal", "Adjust Colo&rs") "...", ACT_ADJUST_TERMINAL_COLORS, ICON_CHOOSE_COLOR, L("AdjustTerminalColors|") + termIdStr);
    MI(TN("pmenu-terminal", "Adjust &Layout") "...", ACT_ADJUST_TERMINAL_LAYOUT, ICON_ADJUST_LAYOUT, L("AdjustTerminalLayout|") + termIdStr);
    MI(TN("pmenu-terminal", "Adjust Scrollback") "...", ACT_ADJUST_TERMINAL_SCROLLBACK, ICON_ADJUST_SCROLLBACK, L("AdjustTerminalScrollback|") + termIdStr);
    MI(TN("pmenu-terminal", "Clear Scrollback and Reset"), ACT_RESET_AND_CLEAR_TERMINAL, ICON_RESET_AND_CLEAR_TERMINAL, L("ResetAndClearTerminal|") + termIdStr);
    MS();
    MI(TN("pmenu-terminal", "&Edit Profile \"%1\""), ACT_EDIT_PROFILE, ICON_EDIT_PROFILE, L("EditProfile"), DynEditProfile);

    SUBMENU_T(term, "dmenu-profile", SwitchProfileMenu, TN("pmenu-terminal", "&Switch Profile"));

    MI(TN("pmenu-terminal", "Edit &Keymap \"%1\""), ACT_EDIT_KEYMAP, ICON_EDIT_KEYMAP, L("EditKeymap"), DynEditKeymap);
    MS();
    MI(TN("pmenu-terminal", "C&lose Terminal"), ACT_CLOSE_TERMINAL, ICON_CLOSE_TERMINAL, L("CloseTerminal|") + termIdStr);
    return menu;
}

QMenu *
MainWindow::getThumbPopup(ThumbArrange *thumb)
{
    auto i = m_popups.constFind(thumb);
    if (i != m_popups.cend())
        return *i;

    TermInstance *term = thumb->term();
    const QString &termIdStr = term->idStr();

    POPUP_TOBJ(term, "pmenu-thumb", thumb);
    menu->enable(SettingsMenu);
    menu->enable(TermMenu);

    MI(TN("pmenu-thumb", "&Paste"), ACT_PASTE, ICON_PASTE, L("Paste|") + termIdStr);
    MS();
    MI(TN("pmenu-thumb", "Clone &Terminal"), ACT_CLONE_TERMINAL, ICON_CLONE_TERMINAL, L("CloneTerminal||") + termIdStr);
    MI(TN("pmenu-thumb", "Duplicate Terminal"), ACT_DUPLICATE_TERMINAL, ICON_DUPLICATE_TERMINAL, L("CloneTerminal|1|") + termIdStr);

    SUBMENU_T(term, "dmenu-icon", TermIconMenu, TN("pmenu-thumb", "Set &Icon"), ICON_CHOOSE_ICON);
    SUBMENU_T(term, "dmenu-alert", AlertMenu, TN("pmenu-thumb", "Set &Alert"), ICON_SET_ALERT);

    SUBMENU_T0(term, "pmenu-thumb-order", TN("pmenu-thumb", "Re&order"), ICON_REORDER);
    SI(TN("pmenu-thumb-order", "Move to &Front"), ACT_REORDER_TERMINAL_FIRST, ICON_REORDER_TERMINAL_FIRST, L("ReorderTerminalFirst|") + termIdStr);
    SI(TN("pmenu-thumb-order", "Move to &Back"), ACT_REORDER_TERMINAL_LAST, ICON_REORDER_TERMINAL_LAST, L("ReorderTerminalLast|") + termIdStr);
    SI(TN("pmenu-thumb-order", "Move &Up"), ACT_REORDER_TERMINAL_FORWARD, ICON_REORDER_TERMINAL_FORWARD, L("ReorderTerminalForward|") + termIdStr);
    SI(TN("pmenu-thumb-order", "Move &Down"), ACT_REORDER_TERMINAL_BACKWARD, ICON_REORDER_TERMINAL_BACKWARD, L("ReorderTerminalBackward|") + termIdStr);

    MI(TN("pmenu-thumb", "&Hide Terminal"), ACT_HIDE_TERMINAL, ICON_HIDE_TERMINAL, L("HideTerminal|") + termIdStr);
    MS();
    MI(TN("pmenu-thumb", "&Input Follower"), ACT_INPUT_TOGGLE_FOLLOWER, ICON_INPUT_TOGGLE_FOLLOWER, L("InputToggleFollower"), DynInputFollower);
    MI(TN("pmenu-thumb", "&Stop Input Multiplexing"), ACT_INPUT_UNSET_LEADER, ICON_INPUT_UNSET_LEADER, L("InputUnsetLeader"), DynInputMultiplexing);
    MS();
    MI(TN("pmenu-thumb", "Adjust Colo&rs") "...", ACT_ADJUST_TERMINAL_COLORS, ICON_CHOOSE_COLOR, L("AdjustTerminalColors|") + termIdStr);
    MI(TN("pmenu-thumb", "Clear Scrollback and Reset"), ACT_RESET_AND_CLEAR_TERMINAL, ICON_RESET_AND_CLEAR_TERMINAL, L("ResetAndClearTerminal|") + termIdStr);
    MS();
    MI(TN("pmenu-thumb", "&Edit Profile \"%1\""), ACT_EDIT_PROFILE, ICON_EDIT_PROFILE, L("EditProfile"), DynEditProfile);

    SUBMENU_T(term, "dmenu-profile", SwitchProfileMenu, TN("pmenu-thumb", "&Switch Profile"));

    MI(TN("pmenu-thumb", "Edit &Keymap \"%1\""), ACT_EDIT_KEYMAP, ICON_EDIT_KEYMAP, L("EditKeymap"), DynEditKeymap);
    MS();
    MI(TN("pmenu-thumb", "&Disconnect Peer"), ACT_DISCONNECT_TERMINAL, ICON_DISCONNECT_TERMINAL, L("DisconnectTerminal|") + termIdStr, DynHasPeer);
    MI(TN("pmenu-thumb", "C&lose Terminal"), ACT_CLOSE_TERMINAL, ICON_CLOSE_TERMINAL, L("CloseTerminal|") + termIdStr);
    return menu;
}

QMenu *
MainWindow::getServerPopup(ServerInstance *server)
{
    auto i = m_popups.constFind(server);
    if (i != m_popups.cend())
        return *i;

    const QString &serverIdStr = server->idStr();

    POPUP_TOBJ(server, "pmenu-server", server);
    menu->enable(ServerMenu);

    MI(TN("pmenu-server", "&New Terminal"), ACT_NEW_TERMINAL, ICON_NEW_TERMINAL, L("NewTerminal||") + serverIdStr);

    SUBMENU_S(server, "dmenu-newterm", NewTermMenu, TN("pmenu-server", "New &Terminal"));
    SUBMENU_S(server, "dmenu-newwin", NewWindowMenu, TN("pmenu-server", "New &Window"), ICON_NEW_WINDOW);

    MS();
    MI(TN("pmenu-server", "&Hide Terminals"), ACT_HIDE_SERVER, ICON_HIDE_SERVER, L("HideServer|") + serverIdStr);
    MI(TN("pmenu-server", "&Show Terminals"), ACT_SHOW_SERVER, ICON_SHOW_SERVER, L("ShowServer|") + serverIdStr);

    SUBMENU_S(server, "dmenu-icon", ServerIconMenu, TN("pmenu-server", "Set &Icon"), ICON_CHOOSE_ICON);

    SUBMENU_S0(server, "pmenu-server-order", TN("pmenu-server", "Re&order"), ICON_REORDER);
    SI(TN("pmenu-server-order", "Move to &Front"), ACT_REORDER_SERVER_FIRST, ICON_REORDER_SERVER_FIRST, L("ReorderServerFirst|") + serverIdStr);
    SI(TN("pmenu-server-order", "Move to &Back"), ACT_REORDER_SERVER_LAST, ICON_REORDER_SERVER_LAST, L("ReorderServerLast|") + serverIdStr);
    SI(TN("pmenu-server-order", "Move &Up"), ACT_REORDER_SERVER_FORWARD, ICON_REORDER_SERVER_FORWARD, L("ReorderServerForward|") + serverIdStr);
    SI(TN("pmenu-server-order", "Move &Down"), ACT_REORDER_SERVER_BACKWARD, ICON_REORDER_SERVER_BACKWARD, L("ReorderServerBackward|") + serverIdStr);

    MS();
    MI(TN("pmenu-server", "&Edit Server \"%1\""), ACT_EDIT_SERVER, ICON_EDIT_SERVER, L("EditServer|") + serverIdStr, DynEditServer);
    MI(TN("pmenu-server", "&Manage Servers") "...", ACT_MANAGE_SERVERS, ICON_MANAGE_SERVERS, L("ManageServers"));
    MI(TN("pmenu-server", "Manage &Connections") "...", ACT_MANAGE_CONNECTIONS, ICON_MANAGE_CONNECTIONS, L("ManageConnections"));
    MS();
    MI(TN("pmenu-server", "&Disconnect"), ACT_DISCONNECT_SERVER, ICON_DISCONNECT_SERVER, L("DisconnectServer|") + serverIdStr);
    return menu;
}

QMenu *
MainWindow::getManagePopup(TermInstance *term, QWidget *parent)
{
    const QString &termIdStr = term->idStr();
    DynamicMenu *subMenu;

    POPUP_TDOH(term, "pmenu-mterm", parent);
    menu->enable(SettingsMenu);
    menu->enable(TermMenu);

    MI(TN("pmenu-mterm", "Clone &Terminal"), ACT_CLONE_TERMINAL, ICON_CLONE_TERMINAL, L("CloneTerminal||") + termIdStr);
    MI(TN("pmenu-mterm", "Duplicate Terminal"), ACT_DUPLICATE_TERMINAL, ICON_DUPLICATE_TERMINAL, L("CloneTerminal|1|") + termIdStr);

    SUBMENU_T(term, "dmenu-icon", TermIconMenu, TN("pmenu-mterm", "Set &Icon"), ICON_CHOOSE_ICON);
    SUBMENU_T(term, "dmenu-alert", AlertMenu, TN("pmenu-mterm", "Set &Alert"), ICON_SET_ALERT);

    SUBMENU_T0(term, "pmenu-mterm-order", TN("pmenu-mterm", "Re&order"), ICON_REORDER);
    SI(TN("pmenu-mterm-order", "Move to &Front"), ACT_REORDER_TERMINAL_FIRST, ICON_REORDER_TERMINAL_FIRST, L("ReorderTerminalFirst|") + termIdStr);
    SI(TN("pmenu-mterm-order", "Move to &Back"), ACT_REORDER_TERMINAL_LAST, ICON_REORDER_TERMINAL_LAST, L("ReorderTerminalLast|") + termIdStr);
    SI(TN("pmenu-mterm-order", "Move &Up"), ACT_REORDER_TERMINAL_FORWARD, ICON_REORDER_TERMINAL_FORWARD, L("ReorderTerminalForward|") + termIdStr);
    SI(TN("pmenu-mterm-order", "Move &Down"), ACT_REORDER_TERMINAL_BACKWARD, ICON_REORDER_TERMINAL_BACKWARD, L("ReorderTerminalBackward|") + termIdStr);

    MI(TN("pmenu-mterm", "&Hide Terminal"), ACT_HIDE_TERMINAL, ICON_HIDE_TERMINAL, L("HideTerminal|") + termIdStr);
    MI(TN("pmenu-mterm", "&Show Terminal"), ACT_SHOW_TERMINAL, ICON_SHOW_TERMINAL, L("ShowTerminal|") + termIdStr);
    MS();

    SUBMENU_T(term, "pmenu-mterm-input", InputMenu, TN("pmenu-mterm", "&Input Multiplexing"));
    SI(TN("pmenu-mterm-input", "Set &Leader"), ACT_INPUT_SET_LEADER, ICON_INPUT_SET_LEADER, L("InputSetLeader|") + termIdStr, DynInputLeader);
    SI(TN("pmenu-mterm-input", "Set &Follower"), ACT_INPUT_TOGGLE_FOLLOWER, ICON_INPUT_TOGGLE_FOLLOWER, L("InputToggleFollower|") + termIdStr, DynInputFollower);
    SS();
    SI(TN("pmenu-mterm-input", "&Stop"), ACT_INPUT_UNSET_LEADER, ICON_INPUT_UNSET_LEADER, L("InputUnsetLeader|") + termIdStr, DynInputMultiplexing);

    MS();
    MI(TN("pmenu-mterm", "Adjust Colo&rs") "...", ACT_ADJUST_TERMINAL_COLORS, ICON_CHOOSE_COLOR, L("AdjustTerminalColors|") + termIdStr);
    MI(TN("pmenu-mterm", "Random Color Theme"), ACT_RANDOM_TERMINAL_THEME, ICON_RANDOM_THEME, L("RandomTerminalTheme|") + termIdStr);
    MI(TN("pmenu-mterm", "Clear Scrollback and Reset"), ACT_RESET_AND_CLEAR_TERMINAL, ICON_RESET_AND_CLEAR_TERMINAL, L("ResetAndClearTerminal|") + termIdStr);
    MS();
    MI(TN("pmenu-mterm", "&Edit Profile \"%1\""), ACT_EDIT_PROFILE, ICON_EDIT_PROFILE, L("EditProfile"), DynEditProfile);

    SUBMENU_T(term, "dmenu-profile", SwitchProfileMenu, TN("pmenu-mterm", "&Switch Profile"));

    MI(TN("pmenu-mterm", "Edit &Keymap \"%1\""), ACT_EDIT_KEYMAP, ICON_EDIT_KEYMAP, L("EditKeymap"), DynEditKeymap);
    MS();
    MI(TN("pmenu-mterm", "&Disconnect Peer"), ACT_DISCONNECT_TERMINAL, ICON_DISCONNECT_TERMINAL, L("DisconnectTerminal|") + termIdStr, DynHasPeer);
    MI(TN("pmenu-mterm", "C&lose Terminal"), ACT_CLOSE_TERMINAL, ICON_CLOSE_TERMINAL, L("CloseTerminal|") + termIdStr);
    return menu;
}

QMenu *
MainWindow::getManagePopup(ServerInstance *server, QWidget *parent)
{
    const QString &serverIdStr = server->idStr();
    DynamicMenu *subMenu;

    POPUP_TDOH(server, "pmenu-mserver", parent);
    menu->enable(ServerMenu);

    MI(TN("pmenu-mserver", "&New Terminal"), ACT_NEW_TERMINAL, ICON_NEW_TERMINAL, L("NewTerminal||") + serverIdStr);

    SUBMENU_S(server, "dmenu-newterm", NewTermMenu, TN("pmenu-mserver", "New &Terminal"));
    SUBMENU_S(server, "dmenu-newwin", NewWindowMenu, TN("pmenu-mserver", "New &Window"), ICON_NEW_WINDOW);

    MS();
    MI(TN("pmenu-mserver", "&Hide Terminals"), ACT_HIDE_SERVER, ICON_HIDE_SERVER, L("HideServer|") + serverIdStr);
    MI(TN("pmenu-mserver", "&Show Terminals"), ACT_SHOW_SERVER, ICON_SHOW_SERVER, L("ShowServer|") + serverIdStr);

    SUBMENU_S(server, "dmenu-icon", ServerIconMenu, TN("pmenu-mserver", "Set &Icon"), ICON_CHOOSE_ICON);

    SUBMENU_S0(server, "pmenu-mserver-order", TN("pmenu-mserver", "Re&order"), ICON_REORDER);
    SI(TN("pmenu-mserver-order", "Move to &Front"), ACT_REORDER_SERVER_FIRST, ICON_REORDER_SERVER_FIRST, L("ReorderServerFirst|") + serverIdStr);
    SI(TN("pmenu-mserver-order", "Move to &Back"), ACT_REORDER_SERVER_LAST, ICON_REORDER_SERVER_LAST, L("ReorderServerLast|") + serverIdStr);
    SI(TN("pmenu-mserver-order", "Move &Up"), ACT_REORDER_SERVER_FORWARD, ICON_REORDER_SERVER_FORWARD, L("ReorderServerForward|") + serverIdStr);
    SI(TN("pmenu-mserver-order", "Move &Down"), ACT_REORDER_SERVER_BACKWARD, ICON_REORDER_SERVER_BACKWARD, L("ReorderServerBackward|") + serverIdStr);

    MS();
    MI(TN("pmenu-mserver", "&Edit Server \"%1\""), ACT_EDIT_SERVER, ICON_EDIT_SERVER, L("EditServer|") + serverIdStr, DynEditServer);
    MI(TN("pmenu-mserver", "&Manage Servers") "...", ACT_MANAGE_SERVERS, ICON_MANAGE_SERVERS, L("ManageServers"));
    MI(TN("pmenu-mserver", "Manage &Connections") "...", ACT_MANAGE_CONNECTIONS, ICON_MANAGE_CONNECTIONS, L("ManageConnections"));
    MS();
    MI(TN("pmenu-mserver", "&Disconnect"), ACT_DISCONNECT_SERVER, ICON_DISCONNECT_SERVER, L("DisconnectServer|") + serverIdStr);
    return menu;
}
