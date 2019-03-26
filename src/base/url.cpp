// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "url.h"
#include "server.h"
#include "file.h"

#include <QFileInfo>
#include <QDir>
#include <QSysInfo>

TermUrl::TermUrl()
{}

TermUrl::TermUrl(const QUrl &url) :
    QUrl(url)
{
}

TermUrl
TermUrl::parse(const QString &str)
{
    TermUrl tu = str.startsWith('/') ? QUrl::fromLocalFile(str) : QUrl(str);

    QString path = tu.QUrl::path();
    QFileInfo fi(QDir::cleanPath(path));

    if (tu.isLocalFile() && fi.isAbsolute()) {
        tu.m_hasFile = tu.m_hasDir = true;
        tu.m_fileIsDir = path.endsWith('/');

        tu.m_dir = fi.path();
        tu.m_path = fi.filePath();
        tu.m_file = fi.fileName();

        if (tu.m_dir != A("/"))
            tu.m_dir += C('/');
        else if (tu.m_file.isEmpty())
            tu.m_file = C('/');
    }

    return tu;
}

TermUrl
TermUrl::construct(const ServerInstance *server, const QString &dir,
                   const TermFile *file)
{
    QString str;

    if (server) {
        QString host = server->host();
        if (!host.isEmpty())
            str = A("//") + host;
    }

    str += dir;

    if (file) {
        str += file->name;
        if (file->isDir())
            str += C('/');
    }

    TermUrl tu;

    if (!str.isEmpty()) {
        tu = QUrl::fromLocalFile(str);

        tu.m_hasDir = true;
        tu.m_dir = dir;

        if (file) {
            tu.m_hasFile = true;
            tu.m_fileIsDir = file->isDir();
        }

        if (str.endsWith('/'))
            str.chop(1);

        tu.m_file = QFileInfo(str).fileName();
        tu.m_path = tu.QUrl::path();
    }

    return tu;
}

QString
TermUrl::hostDir() const
{
    QString h = host();
    if (!h.isEmpty()) {
        return A("//") + h + m_dir;
    } else {
        return m_dir;
    }
}

QString
TermUrl::slashName() const
{
    return m_file != A("/") ? m_file : A("slash");
}

void
TermUrl::clear()
{
    m_fileIsDir = m_hasFile = m_hasDir = false;
    m_path.clear();
    m_dir.clear();
    m_file.clear();
    QUrl::clear();
}

static inline bool
isSameHost(QString h1, QString h2)
{
    QStringRef r1 = h1.leftRef(h1.indexOf('.'));
    QStringRef r2 = h2.leftRef(h2.indexOf('.'));
    return r1.compare(r2, Qt::CaseInsensitive) == 0;
}

bool
TermUrl::checkHost(const ServerInstance *server) const
{
    QString h1 = host();
    QString h2 = server ? server->host() : QString();
    return h1.isEmpty() || isSameHost(h1, h2);
}

bool
TermUrl::checkHost() const
{
    QString h1 = host();
    QString h2 = QSysInfo::machineHostName();
    return h1.isEmpty() || isSameHost(h1, h2);
}

bool
TermUrl::operator==(const TermUrl &o) const
{
    if (scheme() == A("file") && o.scheme() == A("file"))
    {
        if (m_file != o.m_file || m_dir != o.m_dir)
            return false;

        QString h1 = host(), h2 = o.host();
        return h1.isEmpty() || h2.isEmpty() || isSameHost(h1, h2);
    }

    return QUrl::operator==(o);
}

QString
TermUrl::quoted(QString str)
{
    str.replace(A("'"), A("'\\''"));
    return '\'' + str + '\'';
}
