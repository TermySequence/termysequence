// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "thread.h"
#include "status.h"

#include <QMessageBox>

IconThread::IconThread(QObject *parent, IconWorker *target, int type) :
    QThread(parent),
    m_target(target),
    m_type(type)
{
}

void
IconThread::start()
{
    if (m_type == 0) {
        m_box = new QMessageBox(QMessageBox::Information, ICONTOOL_NAME, m_msg.arg(0));
        m_box->setStandardButtons(QMessageBox::NoButton);
        m_box->show();
    }

    connect(this, SIGNAL(finished()), SLOT(handleFinished()));
    startTimer(250);
    QThread::start();
}

void
IconThread::handleFinished()
{
    g_status->progress(g_mtstr);
    delete m_box;
    deleteLater();
}

void
IconThread::timerEvent(QTimerEvent *)
{
    QString msg = m_msg.arg(progress());
    g_status->progress(msg);
    if (m_box)
        m_box->setText(msg);
}

void
IconThread::run()
{
    m_target->run(this);
}
