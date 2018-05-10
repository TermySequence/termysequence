// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "server.h"
#include "conn.h"
#include "term.h"
#include "listener.h"
#include "thumbicon.h"
#include "termformat.h"
#include "settings/global.h"
#include "settings/servinfo.h"
#include "settings/connect.h"

#include <QTimer>
#include <cassert>

inline void
ServerInstance::setup()
{
    m_iconTypes[1] = m_iconTypes[0] = ThumbIcon::ServerType;
    m_indicatorTypes[0] = ThumbIcon::InvalidType;
    m_format = new TermFormat(this);
}

ServerInstance::ServerInstance(ServerConnection *conn) :
    IdBase(Tsq::Uuid::mt, true, g_listener),
    m_conn(conn)
{
    setup();
}

ServerInstance::ServerInstance(const Tsq::Uuid &id, TermInstance *peer,
                               ServerConnection *conn) :
    IdBase(id, false, g_listener),
    m_conn(conn),
    m_peer(peer)
{
    setup();

    if (peer) {
        connect(peer, &QObject::destroyed, this, [this]{ m_peer = nullptr; });
        m_ours = g_listener->idStr() == peer->attributes().value(g_attr_SENDER_ID);
    }
}

ServerInstance::~ServerInstance()
{
    if (m_conninfo)
        m_conninfo->putReference();
    if (m_servinfo)
        m_servinfo->putReference();
}

void
ServerInstance::setActive(ConnectSettings *info)
{
    m_conninfo = info;
    m_conninfo->takeReference();
    m_conninfo->setActive(this);
}

void
ServerInstance::setInactive()
{
    m_servinfo->setInactive();
    if (m_conninfo)
        m_conninfo->setActive(nullptr);
}

void
ServerInstance::clearConnection()
{
    if (m_conn) {
        setInactive();
        m_conn = nullptr;
        m_indicators[0] = A("disconnected");
        m_indicatorTypes[0] = ThumbIcon::IndicatorType;
        emit connectionChanged();
    }
}

void
ServerInstance::unconnect()
{
    if (m_conn == nullptr)
        g_listener->destroyServer(this);
    else if (m_conn == this)
        g_listener->destroyConnection(m_conn);
    else if (m_peer)
        g_listener->pushTermDisconnect(m_peer);
}

void
ServerInstance::setServerInfo(ServerSettings *info, unsigned version, unsigned hops)
{
    assert(!m_servinfo);
    (m_servinfo = info)->takeReference();
    m_version = version;
    m_nHops = hops;

    m_attributes[g_attr_USER] = info->user();
    m_attributes[g_attr_NAME] = info->name();
    m_attributes[g_attr_HOST] = info->host();
    m_format->handleServerInfo();

    if (m_transient)
        info->setLastIcon(m_icons[0] = A("session"));

    connect(m_servinfo, &ServerSettings::iconChanged, this, &ServerInstance::setIcon);
    connect(m_servinfo, &ServerSettings::fullnameChanged, this, &ServerInstance::fullnameChanged);
    setIcon(m_servinfo->icon(), true);
}

void
ServerInstance::setTermCount(unsigned nTerms)
{
    m_nTerms = nTerms;

    if (nTerms == 0) {
        m_populating = false;
        emit ready();
    } else {
        QTimer::singleShot(g_global->populateTime(), this, SLOT(timerCallback()));
    }
}

void
ServerInstance::timerCallback()
{
    if (m_nTerms) {
        m_nTerms = 0;
        m_populating = false;
        emit ready();
    }
}

void
ServerInstance::addTerm(TermInstance *term)
{
    m_terms.append(term);

    if (m_nTerms && --m_nTerms == 0) {
        m_populating = false;
        emit ready();
    }
}

void
ServerInstance::removeTerm(TermInstance *term)
{
    g_listener->removeTerm(term);
    m_terms.removeOne(term);
    term->deleteLater();
}

void
ServerInstance::setAttribute(const QString &key, const QString &value)
{
    auto i = m_attributes.find(key);
    if (i == m_attributes.cend())
        m_attributes.insert(key, value);
    else if (*i != value)
        *i = value;
    else
        return;

    if (key == g_attr_USER) {
        m_servinfo->setUser(value);
    } else if (key == g_attr_NAME) {
        m_servinfo->setName(value);
    } else if (key == g_attr_HOST) {
        m_servinfo->setHost(value);
    } else if (key == g_attr_ICON && !m_transient) {
        setIcon(value, false);
    }

    emit attributeChanged(key, value);
}

void
ServerInstance::removeAttribute(const QString &key)
{
    if (m_attributes.remove(key)) {
        emit attributeRemoved(key);
    }
}

QString
ServerInstance::host() const
{
    return m_servinfo->host();
}

QString
ServerInstance::shortname() const
{
    return m_servinfo->shortname();
}

QString
ServerInstance::longname() const
{
    return m_servinfo->longname();
}

QString
ServerInstance::fullname() const
{
    return m_servinfo->fullname();
}

QString
ServerInstance::getAttribute(const QString &key) const
{
    return m_attributes.value(key, g_str_unknown);
}

void
ServerInstance::setIcon(const QString &icon, bool priority)
{
    if (m_icons[priority] != icon) {
        m_icons[priority] = icon;
        m_servinfo->setLastIcon(icon);
        emit iconsChanged(m_icons, m_iconTypes);
    }
}
