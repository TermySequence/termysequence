// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "statuslink.h"
#include "statuslabel.h"
#include "statusprogress.h"
#include "listener.h"
#include "task.h"
#include "taskmodel.h"
#include "taskstatus.h"

StatusLink::StatusLink(MainWindow *parent) :
    m_parent(parent),
    m_progress(new StatusProgress)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    QString url(A("<a href='https://" PRODUCT_DOMAIN "/user/'>" PRODUCT_DOMAIN "</a>"));
    auto *label = new StatusLabel(url);
    label->setOpenExternalLinks(true);

    m_progress->setMinimum(0);
    m_progress->setMaximum(100);
    m_progress->setTextVisible(false);

    addWidget(label);
    addWidget(m_progress);

    m_sizeHint = label->sizeHint();

    connect(g_listener->taskmodel(),
            SIGNAL(activeTaskChanged(TermTask*)),
            SLOT(handleActiveTaskChanged(TermTask*)));

    handleActiveTaskChanged(g_listener->taskmodel()->activeTask());
    connect(m_progress, SIGNAL(clicked()), SLOT(handleTaskClicked()));
}

void
StatusLink::handleTaskChanged()
{
    m_progress->setValue(m_task->progress());
}

void
StatusLink::handleActiveTaskChanged(TermTask *task)
{
    if (m_task) {
        disconnect(m_mocTask);
    }

    if ((m_task = task)) {
        m_mocTask = connect(task, SIGNAL(taskChanged()), SLOT(handleTaskChanged()));
        handleTaskChanged();
        setCurrentIndex(1);
    } else {
        setCurrentIndex(0);
    }
}

void
StatusLink::handleTaskClicked()
{
    if (m_task)
        (new TaskStatus(m_task, m_parent))->show();
}

QSize
StatusLink::sizeHint() const
{
    return m_sizeHint;
}
