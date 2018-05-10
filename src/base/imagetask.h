// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "downloadtask.h"

struct TermContent;

//
// Download to file
//
class DownloadImageTask final: public DownloadFileTask
{
public:
    DownloadImageTask(TermInstance *term, const TermContent *content,
                      const QString &outfile, bool overwrite);

    void start(TermManager *manager);

    TermTask* clone() const;
};

//
// Download to content
//
class FetchImageTask: public CopyFileTask
{
private:
    void finishFile();

public:
    FetchImageTask(TermInstance *term, const TermContent *content);

    void start(TermManager *manager);

    bool clonable() const;
};

//
// Download to clipboard
//
class CopyImageTask final: public FetchImageTask
{
private:
    void finishFile();

public:
    CopyImageTask(TermInstance *term, const TermContent *content,
                  const QString &format);

    bool clonable() const;
    TermTask* clone() const;
};
