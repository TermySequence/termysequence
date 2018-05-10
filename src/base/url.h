// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QUrl>

class ServerInstance;
struct TermFile;

class TermUrl final: public QUrl
{
private:
    bool m_hasDir = false;
    bool m_hasFile = false;
    bool m_fileIsDir = false;

    QString m_path, m_dir, m_file;

    TermUrl(const QUrl &url);

public:
    TermUrl();

    static TermUrl parse(const QString &str);
    static TermUrl construct(const ServerInstance *server, const QString &dir,
                             const TermFile *file = nullptr);
    void clear();

    inline bool hasDir() const { return m_hasDir; }
    inline bool hasFile() const { return m_hasFile; }
    inline bool fileIsDir() const { return m_fileIsDir; }

    inline QString hostPath() const { return toLocalFile(); }
    QString hostDir() const;

    inline const QString& path() const { return m_path; }
    inline const QString& dir() const { return m_dir; }
    inline const QString& fileName() const { return m_file; }
    QString slashName() const;

    bool checkHost(const ServerInstance *server) const;
    bool checkHost() const;
    bool operator==(const TermUrl &url) const;
};
