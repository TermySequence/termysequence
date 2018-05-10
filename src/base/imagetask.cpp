// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/icons.h"
#include "imagetask.h"
#include "conn.h"
#include "listener.h"
#include "term.h"
#include "contentmodel.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"

#define TR_TASKOBJ1 TL("task-object", "Thumbnail") + ':'
#define TR_TASKOBJ2 TL("task-object", "Content") + ':'
#define TR_TASKOBJ3 TL("task-object", "Inline content")
#define TR_TASKOBJ4 TL("task-object", "Clipboard");
#define TR_TASKTYPE1 TL("task-type", "Download Inline Content")
#define TR_TASKTYPE2 TL("task-type", "Fetch Inline Content")
#define TR_TASKTYPE3 TL("task-type", "Copy Inline Content")

#define BUFSIZE (16 * TERM_PAYLOADSIZE)
#define HEADERSIZE 60
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)
#define WINDOWSIZE 8

//
// Download to file
//
DownloadImageTask::DownloadImageTask(TermInstance *term, const TermContent *content,
                                     const QString &outfile, bool overwrite) :
    DownloadFileTask(term, content->id, outfile, overwrite)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_DOWNLOAD_IMAGE;
    m_sourceStr = content->isinline ? TR_TASKOBJ1 : TR_TASKOBJ2;
    m_sourceStr += content->tu.path();
}

void
DownloadImageTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_DOWNLOAD_IMAGE);
        m.addBytes(m_buf, 48);
        m.addNumber64(m_infile.toULongLong());
        m.addNumberPair(PAYLOADSIZE, WINDOWSIZE);

        m_server->conn()->push(m.resultPtr(), m.length());
        // Change destination to server
        memcpy(m_buf, m_server->id().buf, 16);
    }
}

TermTask *
DownloadImageTask::clone() const
{
    const TermContent *content = m_term->content()->content(m_infile);
    return content ?
        new DownloadImageTask(m_term, content, m_outfile, m_overwrite) :
        nullptr;
}

//
// Download to content
//
FetchImageTask::FetchImageTask(TermInstance *term, const TermContent *content) :
    CopyFileTask(term, content->id)
{
    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_FETCH_IMAGE;
    m_sourceStr = content->isinline ? TR_TASKOBJ1 : TR_TASKOBJ2;
    m_sourceStr += content->tu.path();
    m_sinkStr = TR_TASKOBJ3;

    disableStatusPopup();
}

void
FetchImageTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_DOWNLOAD_IMAGE);
        m.addBytes(m_buf, 48);
        m.addNumber64(m_infile.toULongLong());
        m.addNumberPair(PAYLOADSIZE, WINDOWSIZE);

        m_server->conn()->push(m.resultPtr(), m.length());
        // Change destination to server
        memcpy(m_buf, m_server->id().buf, 16);
    }
}

void
FetchImageTask::finishFile()
{
    m_term->updateImage(m_infile, m_data.data(), m_data.size());
    closefd();
    finish();
}

bool
FetchImageTask::clonable() const
{
    return false;
}

//
// Download to clipboard
//
CopyImageTask::CopyImageTask(TermInstance *term, const TermContent *content,
                             const QString &format) :
    FetchImageTask(term, content)
{
    m_format = format;

    m_typeStr = TR_TASKTYPE3;
    m_typeIcon = ICON_TASKTYPE_COPY_FILE;
    m_sinkStr = TR_TASKOBJ4;
}

void
CopyImageTask::finishFile()
{
    CopyFileTask::finishFile();
}

bool
CopyImageTask::clonable() const
{
    return CopyFileTask::clonable();
}

TermTask *
CopyImageTask::clone() const
{
    const TermContent *content = m_term->content()->content(m_infile);
    return content ?
        new CopyImageTask(m_term, content, m_format) :
        nullptr;
}
