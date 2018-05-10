// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "statusarrange.h"
#include "statusflags.h"
#include "statusinfo.h"
#include "statusprofile.h"

#include <QHBoxLayout>
#include <QEvent>

MainStatus::MainStatus(TermManager *manager) :
    m_minor(new QLabel),
    m_major(new QLabel(this)),
    m_flags(new StatusFlags(manager, this)),
    m_info(new StatusInfo(manager)),
    m_profile(new StatusProfile(manager)),
    m_main(new QWidget(this)),
    m_statusText(g_mtstr)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_minor, 1, Qt::AlignLeft|Qt::AlignVCenter);
    layout->addWidget(m_flags, 0, Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(new QLabel(C('|')), 0, Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(m_info, 0, Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(new QLabel(C('|')), 0, Qt::AlignRight|Qt::AlignVCenter);
    layout->addWidget(m_profile, 0, Qt::AlignRight|Qt::AlignVCenter);
    m_main->setLayout(layout);

    m_minor->setTextFormat(Qt::RichText);
    m_minor->setOpenExternalLinks(true);

    m_major->setTextFormat(Qt::RichText);
    m_major->setOpenExternalLinks(true);
    m_major->hide();
}

void
MainStatus::doPolish()
{
    m_flags->doPolish();
}

void
MainStatus::showMinor(const QString &text)
{
    if (m_permanentText.isEmpty()) {
        m_minor->setText(text);
        m_major->hide();
        m_main->show();

        if (m_minorId == 0) {
            m_minorId = startTimer(MINOR_MESSAGE_TIME);
        }
    }
}

void
MainStatus::clearMinor()
{
    m_minor->setText(m_statusText.back());

    if (m_minorId) {
        killTimer(m_minorId);
        m_minorId = 0;
    }
}

void
MainStatus::showMajor(const QString &text)
{
    m_major->setText(text);
    m_main->hide();
    m_major->show();

    if (m_majorId == 0) {
        m_majorId = startTimer(MAJOR_MESSAGE_TIME);
    }
}

void
MainStatus::clearMajor()
{
    if (m_permanentText.isEmpty()) {
        m_major->hide();
        m_main->show();
    } else {
        m_major->setText(m_permanentText);
    }

    if (m_majorId) {
        killTimer(m_majorId);
        m_majorId = 0;
    }
}

void
MainStatus::showStatus(const QString &text, int priority)
{
    while (m_statusText.size() <= priority)
        m_statusText.append(g_mtstr);

    m_statusText[priority] = text;

    if (m_minorId == 0)
        m_minor->setText(m_statusText.back());
}

void
MainStatus::clearStatus(int priority)
{
    if (m_statusText.size() > priority) {
        m_statusText[priority].clear();

        while(m_statusText.size() > 1 && m_statusText.back().isEmpty())
            m_statusText.pop_back();
    }

    if (m_minorId == 0)
        m_minor->setText(m_statusText.back());
}

void
MainStatus::showPermanent(const QString &text)
{
    m_permanentText = text;

    if (m_majorId) {
        killTimer(m_majorId);
        m_majorId = 0;
    }

    m_major->setText(text);
    m_main->hide();
    m_major->show();
}

void
MainStatus::clearPermanent()
{
    m_permanentText.clear();
    clearMajor();
}

void
MainStatus::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_minorId)
        clearMinor();
    else
        clearMajor();
}

void
MainStatus::relayout()
{
    m_main->setGeometry(rect());
    m_major->setGeometry(rect());
}

void
MainStatus::resizeEvent(QResizeEvent *event)
{
    relayout();
}

bool
MainStatus::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        relayout();
        setSizePolicy(m_main->sizePolicy());
        setMinimumSize(m_main->minimumSize());
        updateGeometry();
        return true;
    }

    return QWidget::event(event);
}

QSize
MainStatus::sizeHint() const
{
    return m_main->sizeHint();
}

QSize
MainStatus::minimumSizeHint() const
{
    return m_main->minimumSizeHint();
}
