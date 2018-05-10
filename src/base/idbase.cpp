// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "idbase.h"
#include "thumbicon.h"

IdBase::IdBase(const Tsq::Uuid &id, bool ours, QObject *parent):
    QObject(parent),
    m_id(id),
    m_idStr(QString::fromLatin1(id.str().c_str())),
    m_ours(ours)
{
}

void
IdBase::changeId(const Tsq::Uuid &id)
{
    m_id = id;
    m_idStr = QString::fromLatin1(id.str().c_str());
}

void
IdBase::setThrottled(bool throttled)
{
    if (m_throttled != throttled)
        emit throttleChanged(m_throttled = throttled);
}

QString
IdBase::iconSpec() const
{
    int i = !m_icons[1].isEmpty();
    return QString::number(m_iconTypes[i]) + m_icons[i];
}

QIcon
IdBase::icon() const
{
    int i = !m_icons[1].isEmpty();
    return ThumbIcon::getIcon((ThumbIcon::IconType)m_iconTypes[i], m_icons[i]);
}

QSvgRenderer *
IdBase::iconRenderer() const
{
    int i = !m_icons[1].isEmpty();
    return (m_icons[i] != TI_NONE_NAME) ?
        ThumbIcon::getRenderer((ThumbIcon::IconType)m_iconTypes[i], m_icons[i]) :
        nullptr;
}

QSvgRenderer *
IdBase::indicatorRenderer(int i) const
{
    return ThumbIcon::getRenderer((ThumbIcon::IconType)m_indicatorTypes[i], m_indicators[i]);
}
