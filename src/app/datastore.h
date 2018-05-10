// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"

#include <QObject>
#include <QHash>
#include <QVector>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE
struct sqlite3;
struct sqlite3_stmt;

#define INVALID_SEARCH_ID 0

#define DS_TABLE_COMMAND        "command"
#define DS_TABLE_ALIAS          "alias"
#define DS_TABLE_EXCLUSION      "exclusion"

#define DS_COLUMN_COMMAND       "command"
#define DS_COLUMN_ACRONYM       "acronym"
#define DS_COLUMN_SCORE         "score"
#define DS_COLUMN_COUNT         "count"
#define DS_COLUMN_STARTED       "started"
#define DS_COLUMN_USER          "user"
#define DS_COLUMN_HOST          "host"
#define DS_COLUMN_PATH          "path"
#define DS_COLUMN_ALIAS         "alias"
#define DS_COLUMN_UPDATED       "updated"
#define DS_COLUMN_REGEXP        "regexp"

#define DS_RESULT_TYPE          "type"

extern const QString g_ds_COMMAND;
extern const QString g_ds_ACRONYM;
extern const QString g_ds_SCORE;
extern const QString g_ds_COUNT;
extern const QString g_ds_STARTED;
extern const QString g_ds_USER;
extern const QString g_ds_HOST;
extern const QString g_ds_PATH;
extern const QString g_ds_ALIAS;
extern const QString g_ds_UPDATED;
extern const QString g_ds_TYPE;

//
// Thread worker
//
struct DatastoreSearch {
    struct sqlite3_stmt *stmt;
    int limit;
    int count;
};

class DatastoreWorker final: public QObject
{
    Q_OBJECT

private:
    struct sqlite3 *m_db;
    struct sqlite3_stmt *m_commandins;
    struct sqlite3_stmt *m_commandscore;
    struct sqlite3_stmt *m_commanddel;
    struct sqlite3_stmt *m_aliasins;
    struct sqlite3_stmt *m_aliasget;
    struct sqlite3_stmt *m_aliasdel;

    int64_t m_origin;

    QHash<unsigned,DatastoreSearch> m_searches;
    QVector<QRegularExpression> m_exclusions;

    int m_timerId;
    QThread *m_thread;

signals:
    void reportResult(unsigned id, AttributeMap resultInfo);
    void reportFinished(unsigned id, bool overlimit);

protected:
    void timerEvent(QTimerEvent *event);

public slots:
    void startSearch(unsigned id, int limit, QString str);
    void stopSearch(unsigned id);

    void storeCommand(AttributeMap commandInfo);
    void removeCommand(QString command);
    void storeAlias(AttributeMap aliasInfo);

    void quit();

public:
    DatastoreWorker(QThread *thread);
    bool initialize(const char *path);
    void teardown();
};

//
// Thread controller
//
class DatastoreController final: public QObject
{
    Q_OBJECT

private:
    QThread *m_thread;
    DatastoreWorker *m_worker;

    unsigned m_nextSearchId = INVALID_SEARCH_ID;
    bool m_haveDb = false;

signals:
    void reportResult(unsigned id, AttributeMap resultInfo);
    void reportFinished(unsigned id, bool overlimit);

    // Internal worker signals
    void sigStartSearch(unsigned id, int limit, QString str);
    void sigStopSearch(unsigned id);
    void sigStoreCommand(AttributeMap commandInfo);
    void sigRemoveCommand(QString command);
    void sigStoreAlias(AttributeMap aliasInfo);
    void sigQuit();

public:
    DatastoreController();
    ~DatastoreController();

    void startSearch(const QString &str, int limit, unsigned &id);
    void stopSearch(unsigned &id);

    void storeCommand(const AttributeMap &commandInfo);
    void removeCommand(const QString &command);
    void storeAlias(const QString &alias, const QString &command);

public:
    static QList<int> getAcronymPositions(const QString &str);
};

extern DatastoreController *g_datastore;
