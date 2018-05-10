// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include <QObject>
#include <QVector>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class MainStatus final: public QObject
{
    Q_OBJECT

public:
    typedef QVector<std::pair<QString,int>> Logbuf;

private:
    QLabel *m_label = nullptr;

    bool m_interactive;
    bool m_loadIssue = false;

    Logbuf m_logbuf;
    QMetaObject::Connection m_mocLog;

private slots:
    void handleLog(QString message, int severity);

signals:
    void sendLog(QString message, int severity);
    void sendProgress(QString message);

public:
    MainStatus(bool interactive);

    QWidget* getWidget();

    void update();

    void log(QString message, int severity = 0);
    inline void progress(QString message) { emit sendProgress(message); }
    Logbuf&& fetchLog();

    inline bool loadIssue() const { return m_loadIssue; }
    inline void reportLoadIssue() { m_loadIssue = true; }
};

extern MainStatus *g_status;
