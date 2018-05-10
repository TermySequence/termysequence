// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "attrbase.h"
#include "iconbutton.h"
#include "settings/global.h"

#include <QDesktopServices>
#include <QUrl>

#define TR_BUTTON1 TL("input-button", "Help")

HelpButton::HelpButton(const char *pagec) :
    IconButton(ICON_HELP_CONTENTS, TR_BUTTON1)
{
    QString path = (pagec[0] != '/') ?
        L("dialogs/%1.html").arg(QString::fromLatin1(pagec)) :
        QString::fromLatin1(pagec + 1) + A(".html");

    const QString url = g_global->docUrl(path);
    setToolTip(url);

    connect(this, &QPushButton::clicked, [url]{
        QDesktopServices::openUrl(url);
    });
}
