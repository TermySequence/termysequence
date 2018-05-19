// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "manager.h"
#include "listener.h"
#include "server.h"
#include "stack.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "task.h"
#include "mainwindow.h"
#include "jobwidget.h"
#include "jobmodel.h"
#include "notewidget.h"
#include "filewidget.h"
#include "suggestwidget.h"
#include "taskwidget.h"
#include "taskmodel.h"
#include "taskstatus.h"
#include "contentmodel.h"
#include "imagetask.h"
#include "uploadtask.h"
#include "misctask.h"
#include "runtask.h"
#include "mounttask.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/launcher.h"
#include "settings/servinfo.h"
#include "settings/profiledialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QDir>

#define TR_ERROR1 TL("error", "The remote path '%1' is not an absolute path")
#define TR_ERROR2 TL("error", "The file's host '%1' does not match the terminal's host '%2'")
#define TR_ERROR3 TL("error", "Cannot open '%1': Remote file mounting is disabled in launcher \"%2\"")
#define TR_FIELD1 TL("input-field", "New file name") + ':'
#define TR_TEXT1 TL("window-text", "Choose destination for download")
#define TR_TEXT2 TL("window-text", "Choose file to upload")
#define TR_TITLE1 TL("window-title", "Rename File")

void
TermManager::actionScrollPromptFirst()
{
    if (m_scrollport)
        m_scrollport->reportPromptRequest(SCROLLREQ_PROMPTFIRST);
}

void
TermManager::actionScrollPromptUp()
{
    if (m_scrollport)
        m_scrollport->reportPromptRequest(SCROLLREQ_PROMPTUP);
}

void
TermManager::actionScrollPromptDown()
{
    if (m_scrollport)
        m_scrollport->reportPromptRequest(SCROLLREQ_PROMPTDOWN);
}

void
TermManager::actionScrollPromptLast()
{
    if (m_scrollport)
        m_scrollport->reportPromptRequest(SCROLLREQ_PROMPTLAST);
}

void
TermManager::actionScrollNoteUp()
{
    if (m_scrollport)
        m_scrollport->reportNoteRequest(SCROLLREQ_NOTEUP);
}

void
TermManager::actionScrollNoteDown()
{
    if (m_scrollport)
        m_scrollport->reportNoteRequest(SCROLLREQ_NOTEDOWN);
}

void
TermManager::actionScrollJobStartContext()
{
    if (m_scrollport)
        m_scrollport->scrollToRegionByClickPoint(true, true);
}

void
TermManager::actionScrollJobEndContext()
{
    if (m_scrollport)
        m_scrollport->scrollToRegionByClickPoint(false, true);
}

bool
TermManager::lookupJob(const QString &regionId, const QString &terminalId, regionid_t &idret)
{
    TermInstance *result;

    if (regionId.isEmpty() && terminalId.isEmpty()) {
        idret = m_scrollport ? m_scrollport->currentJobId() : 0;
        return !!m_scrollport;
    }
    if (regionId == A("Selected")) {
        idret = m_scrollport ? m_scrollport->activeJobId() : 0;
        return !!m_scrollport;
    }

    if (regionId == A("Tool")) {
        result = lastToolTerm();
        idret = lastToolRegion();
    } else {
        result = lookupTerm(terminalId);
        idret = regionId.toUInt();
    }

    if (result && m_stack) {
        m_stack->setTerm(result);
        raiseTerminalsWidget(false);
        if (m_scrollport && m_scrollport->term() == result)
            return true;
    }

    return false;
}

QString
TermManager::lookupCommand(const QString &regionId, const QString &terminalId)
{
    TermInstance *result;
    regionid_t id;
    QString text;

    if (regionId.isEmpty() && terminalId.isEmpty()) {
        result = m_scrollport ? m_term : nullptr;
        id = m_scrollport ? m_scrollport->currentJobId() : 0;
    }
    else if (regionId == A("Selected")) {
        result = m_scrollport ? m_term : nullptr;
        id = m_scrollport ? m_scrollport->activeJobId() : 0;
    }
    else if (regionId == A("Tool")) {
        result = lastToolTerm();
        id = lastToolRegion();
    }
    else {
        result = lookupTerm(terminalId);
        id = regionId.toUInt();
    }

    if (result) {
        const TermBuffers *buffers = result->buffers();
        const Region *region = buffers->safeRegion(id);
        if (region) {
            auto i = region->attributes.constFind(g_attr_REGION_COMMAND);
            if (i != region->attributes.cend())
                text = *i;
            else if (region == buffers->lastRegion(Tsqt::RegionJob))
                text = result->attributes().value(g_attr_COMMAND);
        }
    }
    if (text.isEmpty() && !terminalId.isEmpty()) {
        // Search the job history
        text = g_listener->jobmodel()->findCommand(terminalId.toStdString(),
                                                   regionId.toUInt());
    }

    return text;
}

void
TermManager::actionWriteCommand(QString regionId, QString terminalId)
{
    QString text = lookupCommand(regionId, terminalId);

    if (m_term && !m_term->overlayActive() && !text.isEmpty()) {
        auto widget = m_parent->commandsWidget();
        m_term->pushInput(this, widget->getKillSequence(), false);
        m_term->pushInput(this, text);
    }
}

void
TermManager::actionWriteCommandNewline(QString regionId, QString terminalId)
{
    QString text = lookupCommand(regionId, terminalId);

    if (m_term && !m_term->overlayActive() && !text.isEmpty()) {
        auto widget = m_parent->commandsWidget();
        m_term->pushInput(this, widget->getKillSequence(), false);
        m_term->pushInput(this, text);
        m_term->pushInput(this, C('\n'), false);
    }
}

void
TermManager::actionCopyCommand(QString regionId, QString terminalId)
{
    QString text = lookupCommand(regionId, terminalId);
    QApplication::clipboard()->setText(text);
    reportClipboardCopy(text.size());
}

void
TermManager::actionCopyOutput(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->copyRegionById(Tsqt::RegionOutput, id);
}

void
TermManager::actionCopyJob(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->copyRegionById(Tsqt::RegionJob, id);
}

void
TermManager::actionSelectOutput(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->selectRegionById(Tsqt::RegionOutput, id);
}

void
TermManager::actionSelectCommand(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->selectRegionById(Tsqt::RegionCommand, id);
}

void
TermManager::actionSelectJob(QString regionId, QString terminalId)
{
    regionid_t id;
    if (lookupJob(regionId, terminalId, id))
        m_scrollport->selectRegionById(Tsqt::RegionJob, id);
}

void
TermManager::actionCopyOutputContext()
{
    if (m_scrollport)
        m_scrollport->copyRegionByClickPoint(Tsqt::RegionOutput);
}

void
TermManager::actionCopyCommandContext()
{
    if (m_scrollport)
        m_scrollport->copyRegionByClickPoint(Tsqt::RegionCommand);
}

void
TermManager::actionCopyJobContext()
{
    if (m_scrollport)
        m_scrollport->copyRegionByClickPoint(Tsqt::RegionJob);
}

void
TermManager::actionSelectOutputContext()
{
    if (m_scrollport)
        m_scrollport->selectRegionByClickPoint(Tsqt::RegionOutput, true);
}

void
TermManager::actionSelectCommandContext()
{
    if (m_scrollport)
        m_scrollport->selectRegionByClickPoint(Tsqt::RegionCommand, true);
}

void
TermManager::actionSelectJobContext()
{
    if (m_scrollport)
        m_scrollport->selectRegionByClickPoint(Tsqt::RegionJob, true);
}

void
TermManager::actionToolFilterRemoveClosed()
{
    m_parent->jobsWidget()->removeClosedTerminals();
    m_parent->notesWidget()->removeClosedTerminals();
    m_parent->tasksWidget()->removeClosedTerminals();
}

void
TermManager::actionToolFilterAddTerminal(QString terminalId)
{
    Tsq::Uuid id;
    if (terminalId.isEmpty() && m_term)
        id = m_term->id();
    else
        id.parse(pr(terminalId));

    if (!!id) {
        m_parent->jobsWidget()->addWhitelist(id);
        m_parent->notesWidget()->addWhitelist(id);
        m_parent->tasksWidget()->addWhitelist(id);
    }
}

void
TermManager::actionToolFilterSetTerminal(QString terminalId)
{
    Tsq::Uuid id;
    TermInstance *term;
    if (terminalId.isEmpty() && (term = lastToolTerm()))
        id = term->id();
    else
        id.parse(pr(terminalId));

    if (!!id) {
        m_parent->jobsWidget()->setWhitelist(id);
        m_parent->notesWidget()->setWhitelist(id);
        m_parent->tasksWidget()->setWhitelist(id);
    }
}

void
TermManager::actionToolFilterSetServer(QString serverId)
{
    Tsq::Uuid id;
    ServerInstance *server;
    if (serverId.isEmpty() && (server = lastToolServer()))
        id = server->id();
    else
        id.parse(pr(serverId));

    if (!!id) {
        m_parent->jobsWidget()->setWhitelist(id);
        m_parent->notesWidget()->setWhitelist(id);
        m_parent->tasksWidget()->setWhitelist(id);
    }
}

void
TermManager::actionToolFilterAddServer(QString serverId)
{
    Tsq::Uuid id;
    if (serverId.isEmpty() && m_server)
        id = m_server->id();
    else
        id.parse(pr(serverId));

    if (!!id) {
        m_parent->jobsWidget()->addWhitelist(id);
        m_parent->notesWidget()->addWhitelist(id);
        m_parent->tasksWidget()->addWhitelist(id);
    }
}

void
TermManager::actionToolFilterExcludeTerminal(QString terminalId)
{
    Tsq::Uuid id;
    TermInstance *term;
    if (terminalId.isEmpty() && (term = lastToolTerm()))
        id = term->id();
    else
        id.parse(pr(terminalId));

    if (!!id) {
        m_parent->jobsWidget()->addBlacklist(id);
        m_parent->notesWidget()->addBlacklist(id);
        m_parent->tasksWidget()->addBlacklist(id);
    }
}

void
TermManager::actionToolFilterExcludeServer(QString serverId)
{
    Tsq::Uuid id;
    ServerInstance *server;
    if (serverId.isEmpty() && (server = lastToolServer()))
        id = server->id();
    else
        id.parse(pr(serverId));

    if (!!id) {
        m_parent->jobsWidget()->addBlacklist(id);
        m_parent->notesWidget()->addBlacklist(id);
        m_parent->tasksWidget()->addBlacklist(id);
    }
}

void
TermManager::actionToolFilterIncludeNothing()
{
    m_parent->jobsWidget()->emptyWhitelist();
    m_parent->notesWidget()->emptyWhitelist();
    m_parent->tasksWidget()->emptyWhitelist();
}

void
TermManager::actionToolFilterReset()
{
    m_parent->jobsWidget()->resetFilter();
    m_parent->notesWidget()->resetFilter();
    m_parent->tasksWidget()->resetFilter();
}

bool
TermManager::checkRemotePath(ServerInstance *server, const TermUrl &tu,
                             const QString &path)
{
    if (!tu.hasDir()) {
        errBox(TR_ERROR1.arg(path), m_parent)->show();
        return false;
    }
    if (!tu.checkHost(server)) {
        errBox(TR_ERROR2.arg(tu.host(), server->host()), m_parent)->show();
        return false;
    }
    return true;
}

void
TermManager::postOpen(ServerInstance *server, LaunchSettings *launcher,
                      const QUrl &url, const AttributeMap &subs)
{
    switch (launcher->launchType()) {
    case LaunchDefault:
        QDesktopServices::openUrl(url);
        break;
    case LaunchLocalCommand:
        (new LocalCommandTask(launcher, subs))->start(this);
        break;
    case LaunchRemoteCommand:
        (new RunCommandTask(server, launcher, subs))->start(this);
        break;
    case LaunchLocalTerm:
        launchTerm(g_listener->localServer(), launcher, subs);
        break;
    case LaunchRemoteTerm:
        launchTerm(server, launcher, subs);
        break;
    case LaunchWriteCommand:
        actionWriteTextNewline(launcher->getText(subs), g_mtstr);
        break;
    }
}

void
TermManager::actionOpenUrl(QString launcherName, QString serverId, QString urlStr)
{
    ServerInstance *result = lookupServer(serverId);

    if (!result || urlStr.isEmpty())
        return;

    if (launcherName == g_str_PROMPT_PROFILE) {
        auto *dialog = new LauncherDialog(m_parent, result, urlStr);
        connect(dialog, &LauncherDialog::okayed, this, &TermManager::actionOpenUrl);
        dialog->show();
    }
    else {
        TermUrl tu = TermUrl::parse(urlStr);
        AttributeMap subs;
        subs[A("%U")] = subs[A("%u")] = tu.toString();

        if (tu.isLocalFile()) {
            subs[A("%F")] = subs[A("%f")] = tu.path();
            subs[A("%n")] = tu.fileName();
            subs[A("%d")] = tu.dir();
        }

        postOpen(result, g_settings->launcher(launcherName), tu, subs);
    }
}

void
TermManager::postOpenFile(ServerInstance *server, LaunchSettings *launcher,
                          const TermUrl &tu, AttributeMap subs)
{
    subs[A("%U")] = subs[A("%u")] = tu.toString();
    subs[A("%F")] = subs[A("%f")] = tu.path();
    subs[A("%n")] = tu.fileName();
    subs[A("%d")] = tu.dir();

    postOpen(server, launcher, tu, subs);
}

static AttributeMap
parseSubstitutionsString(const QString &substitutions)
{
    AttributeMap subs;
    QString marker('%');
    for (const auto &sub: substitutions.split('\x1f'))
        if (sub.size() > 1 && sub[1] == '=' && sub[0].isLetterOrNumber())
            subs[marker + sub[0]] = sub.mid(2);
    return subs;
}

void
TermManager::actionOpenFile(QString launcherName, QString serverId, QString filePath,
                            QString substitutions)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = filePath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(filePath);

    if (!checkRemotePath(result, tu, filePath))
        return;

    filePath = tu.path();

    if (launcherName == g_str_PROMPT_PROFILE) {
        auto *dialog = new LauncherDialog(m_parent, result, filePath, substitutions);
        connect(dialog, &LauncherDialog::okayed, this, &TermManager::actionOpenFile);
        dialog->show();
        return;
    }

    AttributeMap subs = parseSubstitutionsString(substitutions);
    LaunchSettings *launcher = g_settings->launcher(launcherName);
    if (!result->local() && launcher->local()) {
        bool ro;
        switch (launcher->mountType()) {
        case MountReadWrite:
            ro = false;
            break;
        case MountReadOnly:
            ro = true;
            break;
        default:
            errBox(TR_ERROR3.arg(filePath, launcherName), m_parent)->show();
            return;
        }

        auto *task = new MountTask(result, filePath, tu.slashName(), ro);
        task->setLauncher(launcher);
        connect(task, &MountTask::ready, this, [=](QString mountPath) {
            postOpenFile(result, launcher, TermUrl::parse(mountPath), subs);
        });
        task->start(this);
    }
    else {
        postOpenFile(result, launcher, tu, subs);
    }
}

void
TermManager::actionMountFile(QString readonly, QString serverId, QString filePath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = filePath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(filePath);

    if (!checkRemotePath(result, tu, filePath))
        return;

    bool ro = readonly == A("1");

    (new MountTask(result, tu.path(), tu.slashName(), ro))->start(this);
}

void
TermManager::postDownloadImage(TermInstance *term, QString contentId,
                               QString localPath, bool overwrite)
{
    const TermContent *content = term->content()->content(contentId);
    if (!content)
        return;
    if (localPath.isEmpty())
        return;

    if (QFileInfo(localPath).isDir()) {
        QDir localDir(localPath);
        localPath = QDir::cleanPath(localDir.absoluteFilePath(contentId));
    }

    (new DownloadImageTask(term, content, localPath, overwrite))->start(this);
}

void
TermManager::actionDownloadImage(QString termId, QString contentId, QString localPath)
{
    TermInstance *result = lookupTerm(termId);
    if (!result)
        return;

    if (localPath.isEmpty())
        localPath = g_str_PROMPT_PROFILE;

    if (localPath == g_str_PROMPT_PROFILE || !QFileInfo(localPath).isReadable())
    {
        auto *box = saveBox(TR_TEXT1, m_parent);
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            postDownloadImage(result, contentId, box->selectedFiles().value(0), true);
        });
        box->show();
        return;
    }
    postDownloadImage(result, contentId, localPath, false);
}

void
TermManager::actionCopyImage(QString format, QString termId, QString contentId)
{
    TermInstance *result = lookupTerm(termId);
    const TermContent *content;

    if (result && (content = result->content()->content(contentId)))
        (new CopyImageTask(result, content, format))->start(this);
}

void
TermManager::actionFetchImage(QString termId, QString contentId)
{
    TermInstance *result = lookupTerm(termId);
    if (result)
        result->fetchImage(contentId, this);
}

void
TermManager::postDownloadFile(ServerInstance *server, QString remotePath,
                              QString localPath, bool overwrite)
{
    if (localPath.isEmpty())
        return;

    if (QFileInfo(localPath).isDir()) {
        QDir localDir(localPath);
        QString remoteFile = QFileInfo(remotePath).fileName();
        localPath = QDir::cleanPath(localDir.absoluteFilePath(remoteFile));
    }

    (new DownloadFileTask(server, remotePath, localPath, overwrite))->start(this);
}

void
TermManager::actionDownloadFile(QString serverId, QString remotePath, QString localPath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = remotePath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(remotePath);

    if (!checkRemotePath(result, tu, remotePath))
        return;

    remotePath = tu.path();

    if (localPath.isEmpty())
        localPath = result->serverInfo()->downloadLocation() != g_str_CURRENT_PROFILE ?
            result->serverInfo()->downloadLocation() :
            g_global->downloadLocation();

    if (localPath == g_str_PROMPT_PROFILE || !QFileInfo(localPath).isReadable())
    {
        auto *box = saveBox(TR_TEXT1, m_parent);
        box->selectFile(tu.fileName());
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            postDownloadFile(result, remotePath, box->selectedFiles().value(0), true);
        });
        box->show();
        return;
    }
    postDownloadFile(result, remotePath, localPath, false);
}

void
TermManager::actionUploadFile(QString serverId, QString remotePath, QString localPath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = remotePath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(remotePath);

    if (!checkRemotePath(result, tu, remotePath))
        return;

    remotePath = tu.path();

    if (localPath.isEmpty())
    {
        auto *box = openBox(TR_TEXT2, m_parent);
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            QString localPath = box->selectedFiles().value(0);
            QString newPath = remotePath;
            if (tu.fileIsDir())
                newPath += '/' + QFileInfo(localPath).fileName();
            (new UploadFileTask(result, localPath, newPath, true))->start(this);
        });
        box->show();
        return;
    }
    if (tu.fileIsDir())
        remotePath += '/' + QFileInfo(localPath).fileName();
    (new UploadFileTask(result, localPath, remotePath, true))->start(this);
}

void
TermManager::postRenameFile(ServerInstance *server, QString oldPath, QString newPath)
{
    if (!newPath.isEmpty()) {
        QFileInfo oldInfo(oldPath);
        QFileInfo newInfo(newPath);

        if (!newInfo.isAbsolute()) {
            QString oldDir = oldPath.left(oldPath.size() - oldInfo.fileName().size());
            newPath = oldDir + newPath;
        }

        (new RenameFileTask(server, oldPath, newPath))->start(this);
    }
}

void
TermManager::actionRenameFile(QString serverId, QString oldPath, QString newPath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = oldPath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(oldPath);

    if (!checkRemotePath(result, tu, oldPath))
        return;

    oldPath = tu.path();

    if (newPath.isEmpty())
    {
        auto *box = textBox(TR_TITLE1, TR_FIELD1, tu.fileName(), m_parent);
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            postRenameFile(result, oldPath, box->textValue());
        });
        box->show();
        return;
    }
    postRenameFile(result, oldPath, newPath);
}

void
TermManager::actionDeleteFile(QString serverId, QString filePath)
{
    ServerInstance *result = lookupServer(serverId);
    if (result) {
        TermUrl tu = filePath.isEmpty() ?
            m_parent->filesWidget()->selectedUrl() :
            TermUrl::parse(filePath);

        if (checkRemotePath(result, tu, filePath))
            (new DeleteFileTask(result, tu.path()))->start(this);
    }
}

void
TermManager::actionUploadToDirectory(QString serverId, QString remoteDir, QString localPath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    auto tu = remoteDir.isEmpty() ?
        m_parent->filesWidget()->currentUrl() :
        TermUrl::parse(remoteDir);

    if (!checkRemotePath(result, tu, remoteDir))
        return;

    remoteDir = tu.path() + '/';

    if (localPath.isEmpty())
    {
        auto *box = openBox(TR_TEXT2, m_parent);
        box->connect(result, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::accepted, this, [=]{
            QString localPath = box->selectedFiles().value(0);
            QString name = QFileInfo(localPath).fileName();
            (new UploadFileTask(result, localPath, remoteDir + name, false))->start(this);
        });
        box->show();
        return;
    }
    remoteDir += QFileInfo(localPath).fileName();
    (new UploadFileTask(result, localPath, remoteDir, false))->start(this);
}

void
TermManager::actionCopyFile(QString format, QString serverId, QString remotePath)
{
    ServerInstance *result = lookupServer(serverId);
    if (!result)
        return;

    TermUrl tu = remotePath.isEmpty() ?
        m_parent->filesWidget()->selectedUrl() :
        TermUrl::parse(remotePath);

    if (checkRemotePath(result, tu, remotePath))
        (new CopyFileTask(result, tu.path(), format))->start(this);
}

void
TermManager::actionRunCommand(QString serverId, QString cmdspec, QString startdir)
{
    ServerInstance *result = lookupServer(serverId);
    if (result && !cmdspec.isEmpty()) {
        LaunchSettings *launcher = new LaunchSettings;
        launcher->setDirectory(startdir);
        launcher->setCommand(cmdspec.split('\x1f'));
        (new RunCommandTask(result, launcher))->start(this);
        launcher->putReference();
    }
}

void
TermManager::actionPopupCommand(QString serverId, QString cmdspec, QString startdir)
{
    ServerInstance *result = lookupServer(serverId);
    if (result && !cmdspec.isEmpty()) {
        QStringList command = cmdspec.split('\x1f');
        LaunchSettings *launcher = new LaunchSettings(command.at(0));
        launcher->setDirectory(startdir);
        launcher->setCommand(command);
        launcher->setOutputType(InOutDialog);
        (new RunCommandTask(result, launcher))->start(this);
        launcher->putReference();
    }
}

void
TermManager::actionLaunchCommand(QString launcherName, QString serverId,
                                 QString substitutions)
{
    ServerInstance *result = lookupServer(serverId);
    LaunchSettings *launcher = g_settings->launcher(launcherName);
    AttributeMap subs = parseSubstitutionsString(substitutions);

    if (result)
        switch (launcher->launchType()) {
        case LaunchLocalCommand:
            (new LocalCommandTask(launcher, subs))->start(this);
            break;
        case LaunchRemoteCommand:
            (new RunCommandTask(result, launcher, subs))->start(this);
            break;
        }
}

void
TermManager::actionCopyFilePath(QString filePath)
{
    if (filePath.isEmpty())
        filePath = m_parent->filesWidget()->selectedUrl().path();

    QApplication::clipboard()->setText(filePath);
    reportClipboardCopy(filePath.size());
}

void
TermManager::actionCopyDirectoryPath(QString dirPath)
{
    if (dirPath.isEmpty())
        dirPath = m_parent->filesWidget()->selectedUrl().dir();

    QApplication::clipboard()->setText(dirPath);
    reportClipboardCopy(dirPath.size());
}

void
TermManager::actionWriteFilePath(QString filePath)
{
    if (filePath.isEmpty())
        filePath = m_parent->filesWidget()->selectedUrl().path();
    if (m_term)
        m_term->pushInput(this, L("'%1' ").arg(filePath));
}

void
TermManager::actionWriteDirectoryPath(QString dirPath)
{
    if (dirPath.isEmpty())
        dirPath = m_parent->filesWidget()->selectedUrl().dir();
    if (m_term)
        m_term->pushInput(this, L("'%1' ").arg(dirPath));
}

void
TermManager::actionSetFileListingFormat(QString formatStr)
{
    int format = formatStr.toInt();
    switch (format) {
    case FilesLong:
    case FilesShort:
        break;
    default:
        format = 0;
        break;
    }

    m_parent->filesWidget()->setFormat(format);
}

void
TermManager::actionToggleFileListingFormat()
{
    m_parent->filesWidget()->toggleFormat();
}

void
TermManager::actionSetFileListingSort(QString spec)
{
    m_parent->filesWidget()->setSort(spec.toInt());
}

inline TermTask *
TermManager::lookupTask(const QString &taskId)
{
    Tsq::Uuid tid;

    if (taskId.isEmpty())
        return m_parent->tasksWidget()->selectedTask();
    else if (tid.parse(taskId.toLatin1().data()))
        return g_listener->taskmodel()->getTask(tid);
    else
        return nullptr;
}

void
TermManager::actionOpenTaskFile(QString launcherName, QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;

    if (file.isEmpty())
        return;
    if (launcherName.isEmpty())
        launcherName = g_global->taskFile();

    if (launcherName == g_str_DESKTOP_LAUNCH)
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
    else
        actionOpenFile(launcherName, g_listener->localServer()->idStr(),
                       file, g_mtstr);
}

void
TermManager::actionOpenTaskDirectory(QString launcherName, QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;
    QFileInfo fileInfo(file);

    if (file.isEmpty())
        return;
    if (launcherName.isEmpty())
        launcherName = g_global->taskDir();

    if (launcherName == g_str_DESKTOP_LAUNCH)
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.path()));
    else
        actionOpenFile(launcherName, g_listener->localServer()->idStr(),
                       fileInfo.path(), g_mtstr);
}

void
TermManager::actionOpenTaskTerminal(QString profileName, QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;

    if (file.isEmpty())
        return;
    if (profileName.isEmpty())
        profileName = g_global->taskProfile();

    LaunchSettings *launcher = new LaunchSettings;
    launcher->setLaunchType(LaunchLocalTerm);
    launcher->setProfile(profileName);

    AttributeMap subs;
    postOpenFile(g_listener->localServer(), launcher, TermUrl::parse(file), subs);
    delete launcher;
}

void
TermManager::actionCopyTaskFile(QString format, QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;

    if (!file.isEmpty())
        actionCopyFile(format, g_listener->localServer()->idStr(), file);
}

void
TermManager::actionCopyTaskFilePath(QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;

    if (!file.isEmpty())
        actionCopyFilePath(file);
}

void
TermManager::actionCopyTaskDirectoryPath(QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;
    QFileInfo fileInfo(file);

    if (!file.isEmpty())
        actionCopyDirectoryPath(fileInfo.path());
}

void
TermManager::actionWriteTaskFilePath(QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;

    if (!file.isEmpty())
        actionWriteFilePath(file);
}

void
TermManager::actionWriteTaskDirectoryPath(QString taskId)
{
    auto task = lookupTask(taskId);
    QString file = task ? task->launchfile() : g_mtstr;
    QFileInfo fileInfo(file);

    if (!file.isEmpty())
        actionWriteDirectoryPath(fileInfo.path());
}

void
TermManager::actionCancelTask(QString taskId)
{
    auto task = lookupTask(taskId);
    if (task)
        task->cancel();
}

void
TermManager::actionRestartTask(QString taskId)
{
    auto task = lookupTask(taskId);
    if (task && task->clonable() && (task = task->clone()))
        task->start(this);
}

void
TermManager::actionInspectTask(QString taskId)
{
    auto task = lookupTask(taskId);
    if (task)
        (new TaskStatus(task, m_parent))->show();
}

void
TermManager::actionRemoveTasks()
{
    m_parent->tasksWidget()->removeClosedTasks();
}

void
TermManager::actionOpenDesktopUrl(QString url)
{
    QDesktopServices::openUrl(url);
}
