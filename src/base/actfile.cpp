// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "manager.h"
#include "listener.h"
#include "conn.h"
#include "stack.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "screen.h"
#include "selection.h"
#include "mainwindow.h"
#include "dockwidget.h"
#include "suggestwidget.h"
#include "searchwidget.h"
#include "pastetask.h"
#include "arrange.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/launcher.h"
#include "settings/servinfo.h"
#include "settings/dropdialog.h"
#include "settings/profiledialog.h"
#include "os/fd.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileInfo>

#define TR_ASK1 TL("question", "File exists. Overwrite?")
#define TR_ERROR1 TL("error", "Failed to save file '%1': %2")
#define TR_TASKOBJ1 TL("task-object", "File") + A(": ")
#define TR_TEXT1 TL("window-text", "Select file to paste into terminal")
#define TR_TEXT2 TL("window-text", "Choose destination for save")
#define TR_TITLE1 TL("window-title", "Confirm Overwrite")

void
TermManager::actionNewWindow(QString profileName, QString serverId)
{
    TermManager *manager = new TermManager;
    g_listener->addManager(manager);

    for (auto server: qAsConst(m_servers))
        manager->addServer(server);
    for (auto term: qAsConst(m_terms))
        manager->addTerm(term);

    if (!profileName.isEmpty())
        manager->actionNewTerminal(profileName, serverId);

    MainWindow *win = new MainWindow(manager);
    win->bringUp();
}

TermInstance *
TermManager::createTerm(ServerInstance *server, ProfileSettings *profile,
                        TermSig *copyfrom, bool duplicate)
{
    profile->activate();
    QSize size = m_stack ? m_stack->calculateTermSize(profile) : profile->termSize();
    Tsq::Uuid termId(true);

    auto *term = new TermInstance(termId, server, profile, size, true, true);
    term->prepare(copyfrom, duplicate);
    server->conn()->addTerm(term);

    if (duplicate)
        g_listener->pushTermDuplicate(term, size, copyfrom->id);
    else
        g_listener->pushTermCreate(term, size);

    return term;
}

void
TermManager::actionNewTerminal(QString profileName, QString serverId)
{
    ServerInstance *server = lookupServer(serverId);
    if (!server || !server->conn()) {
        qCWarning(lcCommand, "Cannot create terminal: not connected");
        return;
    }

    if (profileName == g_str_PROMPT_PROFILE) {
        auto *dialog = new ProfileDialog(server, m_parent);
        connect(dialog, &ProfileDialog::okayed, this, &TermManager::actionNewTerminal);
        dialog->show();
        return;
    }

    auto *profile = server->serverInfo()->profile(profileName);
    raiseTerm(createTerm(server, profile));
}

void
TermManager::actionCloneTerminal(QString duplicate, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (!result) {
        actionNewTerminal(g_settings->defaultProfile()->name(), g_mtstr);
        return;
    }

    ServerInstance *server = result->server();
    if (!server->conn()) {
        qCWarning(lcCommand, "Cannot create terminal: not connected");
        return;
    }

    TermSig sig;
    result->populateSig(&sig);
    raiseTerm(createTerm(server, result->profile(), &sig, duplicate.toInt()));
}

void
TermManager::launchTerm(ServerInstance *server, const LaunchSettings *launcher,
                        const AttributeMap &subs)
{
    if (!server || !server->conn()) {
        qCWarning(lcCommand, "Cannot create terminal: not connected");
        return;
    }

    TermSig sig;
    auto lp = launcher->getParams(subs);
    auto i = subs.find(A("%f"));

    sig.which = TermSig::Nothing;
    sig.extraEnv = lp.env;
    if (!lp.cmd.isEmpty())
        sig.extraAttr[g_attr_PREF_COMMAND] = lp.cmd;
    if (!lp.dir.isEmpty())
        sig.extraAttr[g_attr_PREF_STARTDIR] = lp.dir;
    if (i != subs.end())
        sig.extraAttr[g_attr_PREF_MESSAGE] = TR_TASKOBJ1 + *i + A("\r\n");

    auto *profile = server->serverInfo()->profile(launcher->profile());
    raiseTerm(createTerm(server, profile, &sig));
}

void
TermManager::actionCommandTerminal(QString profileName, QString serverId, QString cmdspec)
{
    LaunchSettings *launcher = new LaunchSettings;
    launcher->setCommand(cmdspec.split('\x1f'));
    launcher->setProfile(profileName);
    launchTerm(lookupServer(serverId), launcher, AttributeMap());
    delete launcher;
}

void
TermManager::actionManpageTerminal(QString manpage)
{
    if (manpage.isEmpty())
        manpage = APP_NAME;

    LaunchSettings *launcher = new LaunchSettings;
    QString shell(A("man '%1'||(echo Closing terminal in 30s;sleep 30)"));
    QStringList command({"/bin/sh", "sh", "-c", shell.arg(manpage) });
    launcher->setCommand(command);
    launchTerm(g_listener->localServer(), launcher, AttributeMap());
    delete launcher;
}

void
TermManager::actionNewLocalTerminal(QString profileName)
{
    ServerInstance *server = g_listener->localServer();

    if (!server || !server->conn()) {
        qCWarning(lcCommand, "Cannot create terminal: not connected");
        return;
    }

    actionNewTerminal(profileName, server->idStr());
}

void
TermManager::actionCloseTerminal(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        result->close(this);
}

void
TermManager::actionCloseWindow()
{
    m_parent->close();
}

void
TermManager::actionQuitApplication()
{
    g_listener->quit();
}

void
TermManager::postSave(QObject *obj, int type, QString localPath, bool overwrite)
{
    if (localPath.isEmpty())
        return;

    std::string tmp;
    QDialog *box;

    if (!overwrite && osFileExists(pr(localPath)))
        switch (g_global->downloadConfig()) {
        case Tsq::TaskOverwrite:
            break;
        case Tsq::TaskAsk:
            box = askBox(TR_TITLE1, TR_ASK1, m_parent);
            box->connect(obj, SIGNAL(destroyed()), SLOT(deleteLater()));
            connect(box, &QDialog::finished, this, [=](int result) {
                if (result == QMessageBox::Yes) {
                    postSave(obj, type, localPath, true);
                }
            });
            box->show();
            return;
        case Tsq::TaskRename:
            tmp = localPath.toStdString();
            osOpenRenamedFile(tmp, 0600, true);
            localPath = QString::fromStdString(tmp);
            break;
        default:
            return;
        }

    QFile file(localPath);
    bool rc = file.open(QFile::WriteOnly);
    if (rc) {
        switch (type) {
        case 0:
            rc = static_cast<TermInstance*>(obj)->buffers()->saveAs(&file);
            break;
        case 1:
            rc = static_cast<TermInstance*>(obj)->screen()->saveAs(&file);
            break;
        case 2:
            rc = localPath.endsWith(A(".png"), Qt::CaseInsensitive) ?
                static_cast<TermArrange*>(obj->parent())->saveAsImage(&file) :
                static_cast<TermScrollport*>(obj)->saveAs(&file);
            break;
        }
    }
    if (!rc) {
        errBox(TR_ERROR1.arg(localPath, file.errorString()), m_parent)->show();
    }
}

void
TermManager::preSave(QObject *obj, int type, QString localPath)
{
    // type: 0=all(obj=term), 1=screen(obj=term), 2=screen(obj=scrollport)

    if (localPath.isEmpty())
        localPath = g_str_PROMPT_PROFILE;

    if (localPath == g_str_PROMPT_PROFILE || !QFileInfo(localPath).isReadable())
    {
        QStringList filters = { A("Text files (*.txt)"), A("All files (*)") };
        if (type == 2)
            filters.insert(1, A("PNG images (*.png)"));

        auto *box = saveBox(TR_TEXT2, m_parent);
        box->setNameFilters(filters);
        box->connect(obj, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            postSave(obj, type, box->selectedFiles().value(0), true);
        });
        box->show();
        return;
    }
    postSave(obj, type, localPath, false);
}

void
TermManager::actionSaveAll(QString localPath, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        preSave(result, 0, localPath);
}

void
TermManager::actionSaveScreen(QString localPath, QString terminalId)
{
    if (!terminalId.isEmpty()) {
        TermInstance *result = lookupTerm(terminalId);
        if (result)
            preSave(result, 1, localPath);
    }
    else if (m_scrollport) {
        preSave(m_scrollport, 2, localPath);
    }
}

void
TermManager::actionCopy(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        auto *selection = result->buffers()->selection();
        reportClipboardCopy(selection->clipboardCopy());

        if (g_global->copySelect())
            selection->clear();
    }
}

void
TermManager::actionCopyAll(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        reportClipboardCopy(result->buffers()->clipboardCopy());
}

void
TermManager::actionCopyScreen(QString format, QString terminalId)
{
    if (!terminalId.isEmpty()) {
        // format ignored here, can only do text
        TermInstance *result = lookupTerm(terminalId);
        if (result)
            reportClipboardCopy(result->screen()->clipboardCopy());
    }
    else if (m_scrollport) {
        if (format == A("png")) {
            static_cast<TermArrange*>(m_scrollport->parent())->copyImage();
            reportClipboardCopy(0, CopiedImage);
        } else {
            reportClipboardCopy(m_scrollport->clipboardCopy());
        }
    }
}

void
TermManager::actionPaste(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result && !result->overlayActive()) {
        QClipboard *clipboard = QApplication::clipboard();
        result->pushInput(this, clipboard->text());
    }
}

void
TermManager::actionPasteSelectBuffer(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result && !result->overlayActive()) {
        QClipboard *clipboard = QApplication::clipboard();
        result->pushInput(this, clipboard->text(QClipboard::Selection));
    }
}

void
TermManager::actionPasteFile(QString terminalId, QString fileName)
{
    TermInstance *result = lookupTerm(terminalId);
    if (!result || result->overlayActive())
        return;

    if (fileName.isEmpty())
    {
        auto *box = openBox(TR_TEXT1, m_parent);
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, [=]{
            QString fileName = box->selectedFiles().value(0);
            if (!fileName.isEmpty()) {
                (new PasteFileTask(result, fileName))->start(this);
            }
        });
        box->show();
        return;
    }
    (new PasteFileTask(result, fileName))->start(this);
}

void
TermManager::performFileDrop(TermInstance *term, ServerInstance *server,
                             const QMimeData *data, int action)
{
    QList<QUrl> urls = data->urls(), localurls;

    for (auto &url: urls)
        if (url.isLocalFile())
            localurls.append(url);

    if (localurls.isEmpty()) {
        if (term) {
            QString tmp;
            for (auto &url: urls)
                tmp.append(L("'%1' ").arg(url.toString()));
            term->pushInput(this, tmp);
        }
        return;
    }

    auto *dialog = new DropDialog(term, server, action, localurls, this);

    if (action == DropAsk) {
        dialog->start();
        dialog->show();
    } else {
        dialog->run();
        dialog->deleteLater();
    }
}

void
TermManager::terminalFileDrop(TermInstance *term, const QMimeData *data)
{
    performFileDrop(term, term->server(), data, term->server()->local() ?
                    g_global->termLocalFile() : g_global->termRemoteFile());
}

void
TermManager::thumbnailFileDrop(TermInstance *term, const QMimeData *data)
{
    performFileDrop(term, term->server(), data, term->server()->local() ?
                    g_global->thumbLocalFile() : g_global->thumbRemoteFile());
}

void
TermManager::serverFileDrop(ServerInstance *server, const QMimeData *data)
{
    performFileDrop(nullptr, server, data, g_global->serverFile());
}

void
TermManager::actionFind()
{
    m_parent->searchDock()->bringUp();
    m_parent->searchWidget()->takeFocus(m_stack ? m_stack->pos() : 0);
}

void
TermManager::actionSearchUp()
{
    m_parent->searchDock()->bringUp();
    if (m_scrollport)
        m_scrollport->startScan(true);
}

void
TermManager::actionSearchDown()
{
    m_parent->searchDock()->bringUp();
    if (m_scrollport)
        m_scrollport->startScan(false);
}

void
TermManager::actionSearchReset()
{
    m_parent->searchWidget()->resetSearch();
}

void
TermManager::actionSelectAll()
{
    if (m_term)
        m_term->buffers()->selection()->selectBuffer();
}

void
TermManager::actionSelectScreen()
{
    if (m_term)
        m_term->buffers()->selection()->selectView(m_scrollport);
}

void
TermManager::actionSelectLine(QString arg)
{
    if (m_scrollport) {
        if (m_scrollport->selecting())
            m_scrollport->buffers()->selection()->selectLine(arg.toInt());
        else
            m_scrollport->selectPreviousLine();
    }
}

void
TermManager::actionSelectHandle(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_HANDLE, arg.toInt());
}

void
TermManager::actionSelectHandleForwardChar(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_FORWARDCHAR, arg.toInt());
}

void
TermManager::actionSelectHandleBackChar(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_BACKCHAR, arg.toInt());
}

void
TermManager::actionSelectHandleForwardWord(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_FORWARDWORD, arg.toInt());
}

void
TermManager::actionSelectHandleBackWord(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_BACKWORD, arg.toInt());
}

void
TermManager::actionSelectHandleDownLine(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_DOWNLINE, arg.toInt());
}

void
TermManager::actionSelectHandleUpLine(QString arg)
{
    if (m_scrollport && m_scrollport->selecting())
        emit m_scrollport->selectRequest(SELECTREQ_UPLINE, arg.toInt());
}

void
TermManager::actionSelectMoveDownLine()
{
    if (m_scrollport)
        m_scrollport->selectNextLine();
}

void
TermManager::actionSelectMoveUpLine()
{
    if (m_scrollport)
        m_scrollport->selectPreviousLine();
}

void
TermManager::actionSelectMoveForwardWord()
{
    if (m_scrollport) {
        if (!m_scrollport->selecting())
            m_scrollport->selectPreviousLine();
        m_scrollport->buffers()->selection()->moveForwardWord();
    }
}

void
TermManager::actionSelectMoveBackWord()
{
    if (m_scrollport) {
        if (!m_scrollport->selecting())
            m_scrollport->selectPreviousLine();
        m_scrollport->buffers()->selection()->moveBackWord();
    }
}

void
TermManager::actionSelectWord(QString index)
{
    if (m_scrollport) {
        if (!m_scrollport->selecting())
            m_scrollport->selectPreviousLine();
        m_scrollport->buffers()->selection()->selectWord(index.toInt());
    }
}

void
TermManager::actionAnnotateScreen(QString row)
{
    if (m_scrollport)
        m_scrollport->annotateLineByRow(row.isEmpty() ? -1 : row.toInt());
}

void
TermManager::actionAnnotateRegion(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->annotateLineById(id);
}

void
TermManager::actionAnnotateOutput(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->annotateRegionById(Tsqt::RegionOutput, id);
}

void
TermManager::actionAnnotateCommand(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->annotateRegionById(Tsqt::RegionCommand, id);
}

void
TermManager::actionAnnotateLineContext()
{
    if (m_scrollport)
        m_scrollport->annotateLineByClickPoint();
}

void
TermManager::actionAnnotateOutputContext()
{
    if (m_scrollport)
        m_scrollport->annotateRegionByClickPoint(Tsqt::RegionOutput);
}

void
TermManager::actionAnnotateCommandContext()
{
    if (m_scrollport)
        m_scrollport->annotateRegionByClickPoint(Tsqt::RegionCommand);
}

void
TermManager::actionAnnotateSelection()
{
    if (m_scrollport)
        m_scrollport->annotateSelection();
}

void
TermManager::actionRemoveNote(QString regionId, QString terminalId)
{
    TermInstance *result;
    regionid_t id;

    if (regionId.isEmpty() && terminalId.isEmpty()) {
        result = lastToolTerm();
        id = lastToolRegion();
    } else {
        result = lookupTerm(terminalId);
        id = regionId.toUInt();
    }

    if (result)
        result->removeAnnotation(id);
}

void
TermManager::actionToggleCommandMode(QString arg)
{
    switch (arg.toInt()) {
    case 0:
        g_listener->setCommandMode(!g_listener->commandMode());
        break;
    case 1:
        g_listener->setCommandMode(true);
        break;
    case 2:
        g_listener->setCommandMode(false);
        break;
    }
}

void
TermManager::actionToggleSelectionMode(QString arg)
{
    if (m_scrollport) {
        int argint = arg.toInt();

        if (argint == 0)
            argint = (m_scrollport->flags() & Tsqt::SelectMode) ? 2 : 1;

        switch (argint) {
        case 1:
            if (!m_scrollport->selecting())
                m_scrollport->selectPreviousLine();
            m_scrollport->setSelectionMode(true);
            break;
        case 2:
            m_scrollport->buffers()->selection()->clear();
            break;
        }
    }
}

void
TermManager::actionWriteSelection()
{
    if (m_scrollport && m_scrollport->selecting()) {
        auto *selection = m_scrollport->buffers()->selection();
        QString text = selection->getLine();

        if (!text.isEmpty()) {
            m_term->pushInput(this, text);
        }

        if (g_global->writeSelect())
            selection->clear();
    }
}

void
TermManager::actionWriteSelectionNewline()
{
    if (m_scrollport && m_scrollport->selecting()) {
        auto *selection = m_scrollport->buffers()->selection();
        QString text = selection->getLine();

        if (!text.isEmpty()) {
            m_term->pushInput(this, text);
            m_term->pushInput(this, C('\n'), false);
        }

        if (g_global->writeSelect())
            selection->clear();
    }
}

void
TermManager::actionWriteText(QString text, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        auto widget = m_parent->commandsWidget();
        result->pushInput(this, widget->getKillSequence(), false);
        result->pushInput(this, text);
    }
}

void
TermManager::actionWriteTextNewline(QString text, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        auto widget = m_parent->commandsWidget();
        result->pushInput(this, widget->getKillSequence(), false);
        result->pushInput(this, text);
        result->pushInput(this, C('\n'), false);
    }
}

void
TermManager::actionWriteSuggestion(QString index)
{
    int idx = index.isEmpty() ? -1 : index.toInt();
    auto widget = m_parent->commandsWidget();
    const QString &result = widget->getCommand(idx);

    if (m_term && !result.isEmpty()) {
        m_term->pushInput(this, widget->getKillSequence(), false);
        m_term->pushInput(this, result);
    }
}

void
TermManager::actionWriteSuggestionNewline(QString index)
{
    int idx = index.isEmpty() ? -1 : index.toInt();
    auto widget = m_parent->commandsWidget();
    const QString &result = widget->getCommand(idx);

    if (m_term && !result.isEmpty()) {
        m_term->pushInput(this, widget->getKillSequence(), false);
        m_term->pushInput(this, result);
        m_term->pushInput(this, C('\n'), false);
    }
}

void
TermManager::actionCopySuggestion(QString index)
{
    int idx = index.isEmpty() ? -1 : index.toInt();
    const QString &result = m_parent->commandsWidget()->getCommand(idx);
    QApplication::clipboard()->setText(result);
    reportClipboardCopy(result.size());
}

void
TermManager::actionRemoveSuggestion(QString index)
{
    int idx = index.isEmpty() ? -1 : index.toInt();
    m_parent->commandsWidget()->removeCommand(idx);
}

void
TermManager::actionSuggestFirst()
{
    m_parent->commandsDock()->bringUp();
    m_parent->commandsWidget()->selectFirst();
}

void
TermManager::actionSuggestPrevious()
{
    m_parent->commandsDock()->bringUp();
    m_parent->commandsWidget()->selectPrevious();
}

void
TermManager::actionSuggestNext()
{
    m_parent->commandsDock()->bringUp();
    m_parent->commandsWidget()->selectNext();
}

void
TermManager::actionSuggestLast()
{
    m_parent->commandsDock()->bringUp();
    m_parent->commandsWidget()->selectLast();
}

void
TermManager::actionCopyRegion(QString regionId, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        auto *region = result->buffers()->safeRegion(regionId.toUInt());
        if (region)
            reportClipboardCopy(region->clipboardCopy());
    }
}

void
TermManager::actionCopySemantic(QString regionId, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        auto *region = result->buffers()->safeSemantic(regionId.toUInt());
        if (region)
            reportClipboardCopy(region->clipboardCopy());
    }
}

void
TermManager::actionCopyUrl(QString urlStr)
{
    QString text = QUrl(urlStr).toString();
    QApplication::clipboard()->setText(text);
    reportClipboardCopy(text.size());
}

void
TermManager::actionSetSelectedUrl(QString url)
{
    if (m_scrollport)
        m_scrollport->setSelectedUrl(TermUrl::parse(url));
}
