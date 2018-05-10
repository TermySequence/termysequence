// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "status.h"
#include "iconmodel.h"
#include "workmodel.h"
#include "settings.h"

#include <QLabel>
#include <QTime>
#include <cstdio>

MainStatus *g_status;

MainStatus::MainStatus(bool interactive) :
    m_interactive(interactive)
{
    m_mocLog = connect(this, &MainStatus::sendLog, this, &MainStatus::handleLog);
}

void
MainStatus::log(QString message, int severity)
{
    emit sendLog(QTime::currentTime().toString(L("HH:mm:ss ")) + message, severity);
}

void
MainStatus::handleLog(QString message, int severity)
{
    if (m_interactive)
        m_logbuf.append(std::make_pair(message, severity));
    else
        fprintf(stderr, "%s\n", pr(message));
}

MainStatus::Logbuf &&
MainStatus::fetchLog()
{
    disconnect(m_mocLog);
    return std::move(m_logbuf);
}

void
MainStatus::update()
{
    if (m_label) {
        QStringList texts;
        for (const auto &i: g_settings->sources())
            texts.append(L("%1:%2").arg(i.name).arg(g_iconmodel->themeCount(i.name)));

        texts.append(C('|'));
        texts.append(L("Hidden:%1").arg(g_iconmodel->hiddenCount()));
        texts.append(L("Invalid:%1").arg(g_iconmodel->invalidCount()));
        texts.append(C('|'));
        texts.append(L("Definitions:%1").arg(g_workmodel->rowCount()));

        m_label->setText(texts.join(' '));
    }
}

QWidget *
MainStatus::getWidget()
{
    m_label = new QLabel;
    update();
    return m_label;
}
