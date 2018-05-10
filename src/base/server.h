// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "idbase.h"

class ServerConnection;
class ServerSettings;
class ConnectSettings;
class TermInstance;

class ServerInstance: public IdBase
{
    Q_OBJECT
    Q_PROPERTY(QString icon READ iconStr WRITE setIcon)

private:
    ServerConnection *m_conn;
    ServerSettings *m_servinfo = nullptr;
    TermInstance *m_peer = nullptr;

    QList<TermInstance*> m_terms;

    QString m_cwd;

    unsigned m_version;
    unsigned m_nHops;
    unsigned m_nTerms;

    void setup();

protected:
    bool m_transient = false;
    bool m_persistent = false;
    bool m_populating = true;

    ConnectSettings *m_conninfo = nullptr;

    ServerInstance(ServerConnection *conn);

private slots:
    void timerCallback();

signals:
    void ready();
    void connectionChanged();
    void fullnameChanged(QString fullname);

public:
    ServerInstance(const Tsq::Uuid &id, TermInstance *peer, ServerConnection *conn);
    ~ServerInstance();

    void setTermCount(unsigned nTerms);
    void setServerInfo(ServerSettings *info, unsigned version, unsigned hops);
    void setActive(ConnectSettings *conninfo);
    void setInactive();
    void clearConnection();
    void unconnect();

    inline ServerConnection* conn() { return m_conn; }
    inline TermInstance* peer() { return m_peer; }
    inline ServerSettings* serverInfo() { return m_servinfo; }

    inline const QString& cwd() const { return m_cwd; }
    inline unsigned version() const { return m_version; }
    inline unsigned nHops() const { return m_nHops; }

    inline bool local() const { return m_transient || m_persistent; }
    inline bool transient() const { return m_transient; }
    inline bool persistent() const { return m_persistent; }
    inline bool populating() const { return m_populating; }

    inline const auto& terms() const { return m_terms; }

    void addTerm(TermInstance *term);
    void removeTerm(TermInstance *term);

    void setAttribute(const QString &key, const QString &value);
    void removeAttribute(const QString &key);
    QString getAttribute(const QString &key) const;
    QString host() const;
    QString shortname() const;
    QString longname() const;
    QString fullname() const;

    inline void setCwd(const QString &cwd) { m_cwd = cwd; }
    void setIcon(const QString &icon, bool priority = true);
};
