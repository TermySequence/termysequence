// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "datastore.h"
#include "attr.h"
#include "config.h"
#include "logging.h"
#include "os/time.h"

#include <QRegularExpression>
#include <QDir>
#include <QThread>
#include <cmath>
#include <sqlite3.h>

#define CUSTOM_FUNCTION "datastore_command_match"

#define TABLE_CREATE \
    "CREATE TABLE IF NOT EXISTS " DS_TABLE_COMMAND "(" \
    DS_COLUMN_COMMAND " TEXT PRIMARY KEY NOT NULL, " \
    DS_COLUMN_ACRONYM " TEXT NOT NULL, " \
    DS_COLUMN_SCORE   " REAL NOT NULL, " \
    DS_COLUMN_COUNT   " INTEGER NOT NULL, " \
    DS_COLUMN_STARTED " INTEGER NOT NULL, " \
    DS_COLUMN_USER    " TEXT, " \
    DS_COLUMN_HOST    " TEXT, " \
    DS_COLUMN_PATH    " TEXT) WITHOUT ROWID;" \
    \
    "CREATE INDEX IF NOT EXISTS " DS_TABLE_COMMAND "_score " \
    "ON " DS_TABLE_COMMAND "(" DS_COLUMN_SCORE " DESC);" \
    \
    "CREATE TABLE IF NOT EXISTS " DS_TABLE_ALIAS "(" \
    DS_COLUMN_ALIAS   " TEXT PRIMARY KEY NOT NULL, " \
    DS_COLUMN_COMMAND " TEXT NOT NULL, " \
    DS_COLUMN_UPDATED " INTEGER NOT NULL) WITHOUT ROWID;" \
    \
    "CREATE TABLE IF NOT EXISTS " DS_TABLE_EXCLUSION "(" \
    DS_COLUMN_REGEXP  " TEXT PRIMARY KEY NOT NULL) WITHOUT ROWID;"

#define COMMAND_INSERT \
    "INSERT OR REPLACE INTO " DS_TABLE_COMMAND " " \
    "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);"

#define COMMAND_REMOVE \
    "DELETE FROM " DS_TABLE_COMMAND " " \
    "WHERE " DS_COLUMN_COMMAND " = ?1;"

#define COMMAND_SCORE \
    "SELECT " DS_COLUMN_SCORE ", " DS_COLUMN_COUNT " " \
    "FROM " DS_TABLE_COMMAND " " \
    "WHERE " DS_COLUMN_COMMAND " = ?1;"

#define COMMAND_SEARCH \
    "SELECT * FROM " DS_TABLE_COMMAND " " \
    "WHERE " CUSTOM_FUNCTION "(?1, " DS_COLUMN_COMMAND ", " DS_COLUMN_ACRONYM ") " \
    "ORDER BY " DS_COLUMN_SCORE " DESC;"

#define ALIAS_INSERT \
    "INSERT OR REPLACE INTO " DS_TABLE_ALIAS " " \
    "VALUES(?1, ?2, ?3);"

#define ALIAS_REMOVE \
    "DELETE FROM " DS_TABLE_ALIAS " " \
    "WHERE " DS_COLUMN_COMMAND " = ?1;"

#define ALIAS_SEARCH \
    "SELECT * FROM " DS_TABLE_ALIAS " " \
    "WHERE " DS_COLUMN_ALIAS " = ?1;"

#define TIMEROOT_INSERT \
    "INSERT INTO " DS_TABLE_COMMAND " " \
    "VALUES('', '', 0.0, 0, ?1, NULL, NULL, NULL);"

#define TIMEROOT_FETCH \
    "SELECT " DS_COLUMN_STARTED " FROM " DS_TABLE_COMMAND " " \
    "WHERE " DS_COLUMN_COMMAND " = '';"

#define EXCLUSION_FETCH \
    "SELECT " DS_COLUMN_REGEXP " FROM " DS_TABLE_EXCLUSION ";"

DatastoreController *g_datastore;

const QString g_ds_COMMAND(L(DS_COLUMN_COMMAND));
const QString g_ds_ACRONYM(L(DS_COLUMN_ACRONYM));
const QString g_ds_SCORE(L(DS_COLUMN_SCORE));
const QString g_ds_COUNT(L(DS_COLUMN_COUNT));
const QString g_ds_STARTED(L(DS_COLUMN_STARTED));
const QString g_ds_USER(L(DS_COLUMN_USER));
const QString g_ds_HOST(L(DS_COLUMN_HOST));
const QString g_ds_PATH(L(DS_COLUMN_PATH));
const QString g_ds_ALIAS(L(DS_COLUMN_ALIAS));
const QString g_ds_UPDATED(L(DS_COLUMN_UPDATED));
const QString g_ds_TYPE(L(DS_RESULT_TYPE));

// Note: single character matching only
static const QRegularExpression s_acronymRe(L("[ \\.\\|;<>-]"));

//
// Thread worker
//
DatastoreWorker::DatastoreWorker(QThread *thread) :
    m_db(nullptr),
    m_commandins(nullptr),
    m_commandscore(nullptr),
    m_commanddel(nullptr),
    m_aliasins(nullptr),
    m_aliasget(nullptr),
    m_aliasdel(nullptr),
    m_timerId(0),
    m_thread(thread)
{
}

void
DatastoreWorker::timerEvent(QTimerEvent *)
{
    AttributeMap map;

    for (auto i = m_searches.begin(); i != m_searches.end(); )
    {
        if (sqlite3_step(i->stmt) == SQLITE_ROW)
        {
            QString cmd = (const char *)sqlite3_column_text(i->stmt, 0);
            map[g_ds_COMMAND] = cmd;
            map[g_ds_ACRONYM] = (const char *)sqlite3_column_text(i->stmt, 1);
            map[g_ds_SCORE] = (const char *)sqlite3_column_text(i->stmt, 2);
            map[g_ds_COUNT] = (const char *)sqlite3_column_text(i->stmt, 3);
            map[g_ds_STARTED] = (const char *)sqlite3_column_text(i->stmt, 4);
            map[g_ds_USER] = (const char *)sqlite3_column_text(i->stmt, 5);
            map[g_ds_HOST] = (const char *)sqlite3_column_text(i->stmt, 6);
            map[g_ds_PATH] = (const char *)sqlite3_column_text(i->stmt, 7);
            emit reportResult(i.key(), map);

            i->count += cmd.size();
            if (i->count < i->limit) {
                ++i;
                continue;
            }
        }

        sqlite3_finalize(i->stmt);
        emit reportFinished(i.key(), i->count >= i->limit);
        i = m_searches.erase(i);
    }

    if (m_searches.isEmpty() && m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

void
DatastoreWorker::startSearch(unsigned id, int limit, QString str)
{
    QByteArray bytes = str.toUtf8();
    sqlite3_stmt *stmt;
    int rc;

    auto i = m_searches.constFind(id);
    if (i != m_searches.cend()) {
        stmt = i->stmt;
        sqlite3_reset(stmt);
    } else {
        rc = sqlite3_prepare_v2(m_db, COMMAND_SEARCH, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
            goto err;
    }

    sqlite3_bind_text(stmt, 1, bytes.data(), -1, SQLITE_TRANSIENT);
    m_searches[id] = { stmt, limit, 0 };

    if (m_timerId == 0) {
        m_timerId = startTimer(0);
    }

    // Check the alias table
    sqlite3_bind_text(m_aliasget, 1, bytes.data(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(m_aliasget) == SQLITE_ROW) {
        AttributeMap map;
        map[g_ds_ALIAS] = (const char *)sqlite3_column_text(m_aliasget, 0);
        map[g_ds_COMMAND] = (const char *)sqlite3_column_text(m_aliasget, 1);
        map[g_ds_UPDATED] = (const char *)sqlite3_column_text(m_aliasget, 2);
        map[g_ds_TYPE] = g_ds_ALIAS;
        emit reportResult(id, map);
    }
    sqlite3_reset(m_aliasget);
    return;
err:
    sqlite3_finalize(stmt);
}

void
DatastoreWorker::stopSearch(unsigned id)
{
    auto i = m_searches.constFind(id);
    if (i != m_searches.cend()) {
        sqlite3_finalize(i->stmt);
        m_searches.remove(id);

        if (m_searches.isEmpty() && m_timerId) {
            killTimer(m_timerId);
            m_timerId = 0;
        }
    }
}

QList<int>
DatastoreController::getAcronymPositions(const QString &str)
{
    QList<int> result;
    int n = 0, pos = 0;

    for (auto &i: str.split(s_acronymRe, QString::KeepEmptyParts)) {
        for (auto k = i.cbegin(), l = i.cend(); k != l; ++k) {
            if (k->isLetterOrNumber()) {
                result.append(pos);
                pos += (l - k);
                break;
            }
            ++pos;
        }

        if (++n == DATASTORE_ACRONYM_LIMIT)
            break;

        ++pos; // single char separator
    }

    return result;
}

static inline QByteArray
makeAcronym(const QString &command)
{
    QString result;
    int n = 0;

    for (auto &i: command.split(s_acronymRe, QString::SkipEmptyParts)) {
        for (auto k: i)
            if (k.isLetterOrNumber()) {
                result.append(k);
                break;
            }

        if (++n == DATASTORE_ACRONYM_LIMIT)
            break;
    }

    return result.toUtf8();
}

void
DatastoreWorker::storeCommand(AttributeMap map)
{
    const QString commandStr = map[g_attr_REGION_COMMAND].trimmed();
    if (commandStr.isEmpty())
        return;

    // check exclusions
    for (const auto &re: qAsConst(m_exclusions))
        if (re.match(commandStr).hasMatch())
            return;

    QByteArray command = commandStr.toUtf8();
    QByteArray acronym = makeAcronym(commandStr);
    int64_t started = map[g_attr_REGION_STARTED].toLongLong() / 1000;
    double score = 0.0;
    int64_t count = 1;

    // get score and count
    sqlite3_bind_text(m_commandscore, 1, command.data(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(m_commandscore) == SQLITE_ROW) {
        score = sqlite3_column_double(m_commandscore, 0);
        count = sqlite3_column_int64(m_commandscore, 1) + 1;
    }
    sqlite3_reset(m_commandscore);

    // update score
    double tmp = (started - m_origin) / 3739465.54598419;
    score = log(exp(score - tmp) + 1.0) + tmp;

    // store command with new data
    sqlite3_bind_text(m_commandins, 1, command.data(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(m_commandins, 2, acronym.data(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(m_commandins, 3, score);
    sqlite3_bind_int64(m_commandins, 4, count);
    sqlite3_bind_int64(m_commandins, 5, started);

    auto i = map.constFind(g_attr_REGION_USER), j = map.cend();
    sqlite3_bind_text(m_commandins, 6, i != j ? pr(*i) : NULL, -1, SQLITE_TRANSIENT);
    i = map.constFind(g_attr_REGION_HOST);
    sqlite3_bind_text(m_commandins, 7, i != j ? pr(*i) : NULL, -1, SQLITE_TRANSIENT);
    i = map.constFind(g_attr_REGION_PATH);
    sqlite3_bind_text(m_commandins, 8, i != j ? pr(*i) : NULL, -1, SQLITE_TRANSIENT);

    sqlite3_step(m_commandins);
    sqlite3_reset(m_commandins);
}

void
DatastoreWorker::removeCommand(QString command)
{
    sqlite3_bind_text(m_aliasdel, 1, pr(command), -1, SQLITE_TRANSIENT);
    sqlite3_step(m_aliasdel);
    sqlite3_reset(m_aliasdel);

    sqlite3_bind_text(m_commanddel, 1, pr(command), -1, SQLITE_TRANSIENT);
    sqlite3_step(m_commanddel);
    sqlite3_reset(m_commanddel);
}

void
DatastoreWorker::storeAlias(AttributeMap map)
{
    int64_t updated = map[g_ds_UPDATED].toLongLong() / 1000;

    sqlite3_bind_text(m_aliasins, 1, pr(map[g_ds_ALIAS]), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(m_aliasins, 2, pr(map[g_ds_COMMAND]), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(m_aliasins, 3, updated);

    sqlite3_step(m_aliasins);
    sqlite3_reset(m_aliasins);
}

void
DatastoreWorker::quit()
{
    if (m_timerId)
        killTimer(m_timerId);

    m_thread->quit();
}

extern "C" void
customMatchFunc(sqlite3_context *ctx, int count, sqlite3_value **values)
{
    const char *str = (const char *)sqlite3_value_text(values[0]);
    const char *command = (const char *)sqlite3_value_text(values[1]);
    const char *acronym = (const char *)sqlite3_value_text(values[2]);

    int result = strstr(command, str) || strstr(acronym, str);
    sqlite3_result_int(ctx, result);
}

bool
DatastoreWorker::initialize(const char *path)
{
    sqlite3_stmt *stmt;
    sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);

    int rc = sqlite3_open(path, &m_db);
    if (rc != SQLITE_OK) {
        qCWarning(lcSettings, "Failed to open %s: %s", path, sqlite3_errstr(rc));
        goto err;
    }

    // Register custom function
    rc = sqlite3_create_function(m_db, CUSTOM_FUNCTION, 3,
                                 SQLITE_UTF8|SQLITE_DETERMINISTIC,
                                 NULL, customMatchFunc, NULL, NULL);
    if (rc != SQLITE_OK) {
        qCCritical(lcSettings, "Failed to register new function: %s", sqlite3_errstr(rc));
        goto err;
    }

    // Create tables
    rc = sqlite3_exec(m_db, TABLE_CREATE, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        qCCritical(lcSettings, "Failed to create database tables: %s", sqlite3_errstr(rc));
        goto err;
    }

    // Load the root start time
    rc = sqlite3_prepare_v2(m_db, TIMEROOT_FETCH, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        m_origin = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
    } else if (rc == SQLITE_DONE) {
        m_origin = osWalltime() / 1000;
        sqlite3_finalize(stmt);

        // Insert the root start time
        rc = sqlite3_prepare_v2(m_db, TIMEROOT_INSERT, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
            goto err2;
        sqlite3_bind_int64(stmt, 1, m_origin);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        qCCritical(lcSettings, "Failed to load origin time from database");
        sqlite3_finalize(stmt);
        goto err;
    }

    // Load the exclusions list
    rc = sqlite3_prepare_v2(m_db, EXCLUSION_FETCH, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    for (int i = 0; i < DATASTORE_EXCLUSION_LIMIT; ++i) {
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW)
            break;

        QRegularExpression re((const char *)sqlite3_column_text(stmt, 0));
        if (re.isValid())
            m_exclusions.append(re);
    }
    sqlite3_finalize(stmt);

    // Prepare SQL statements
    rc = sqlite3_prepare_v2(m_db, COMMAND_INSERT, -1, &m_commandins, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_prepare_v2(m_db, COMMAND_SCORE, -1, &m_commandscore, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_prepare_v2(m_db, COMMAND_REMOVE, -1, &m_commanddel, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_prepare_v2(m_db, ALIAS_INSERT, -1, &m_aliasins, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_prepare_v2(m_db, ALIAS_SEARCH, -1, &m_aliasget, NULL);
    if (rc != SQLITE_OK)
        goto err2;
    rc = sqlite3_prepare_v2(m_db, ALIAS_REMOVE, -1, &m_aliasdel, NULL);
    if (rc != SQLITE_OK)
        goto err2;

    return true;
err2:
    qCCritical(lcSettings, "Failed to prepare SQL statement: %s", sqlite3_errstr(rc));
err:
    teardown();
    return false;
}

void
DatastoreWorker::teardown()
{
    for (auto &&i: qAsConst(m_searches))
        sqlite3_finalize(i.stmt);

    sqlite3_finalize(m_aliasdel);
    sqlite3_finalize(m_aliasget);
    sqlite3_finalize(m_aliasins);
    sqlite3_finalize(m_commanddel);
    sqlite3_finalize(m_commandscore);
    sqlite3_finalize(m_commandins);
    sqlite3_close(m_db);
}

//
// Thread controller
//
DatastoreController::DatastoreController() :
    m_thread(new QThread(this)),
    m_worker(new DatastoreWorker(m_thread))
{
    QString path(getenv("XDG_CACHE_HOME"));

    if (path.isEmpty()) {
        QString homePath(getenv("HOME"));
        if (!homePath.isEmpty())
            path = L("%1/.cache").arg(homePath);
    }

    QDir dir(path);

    if (path.isEmpty()) {
        qCWarning(lcSettings) << "Neither XDG_CACHE_HOME nor HOME are defined";
        return;
    } else if (!dir.isAbsolute()) {
        qCWarning(lcSettings) << "Cache folder is not an absolute path";
        return;
    } else if (!dir.exists(APP_NAME) && !dir.mkpath(APP_NAME)) {
        qCWarning(lcSettings) << "Failed to create cache folder";
        return;
    }

    path.append(L("/" APP_NAME "/" APP_DATASTORE));

    // Prepare SQL statements
    if (m_worker->initialize(pr(path)))
    {
        m_haveDb = true;
        m_worker->moveToThread(m_thread);
        connect(this, &DatastoreController::sigStartSearch, m_worker, &DatastoreWorker::startSearch);
        connect(this, &DatastoreController::sigStoreCommand, m_worker, &DatastoreWorker::storeCommand);
        connect(this, &DatastoreController::sigRemoveCommand, m_worker, &DatastoreWorker::removeCommand);
        connect(this, &DatastoreController::sigStoreAlias, m_worker, &DatastoreWorker::storeAlias);
        connect(this, &DatastoreController::sigQuit, m_worker, &DatastoreWorker::quit);

        connect(m_worker, SIGNAL(reportResult(unsigned,AttributeMap)), SIGNAL(reportResult(unsigned,AttributeMap)));
        connect(m_worker, SIGNAL(reportFinished(unsigned,bool)), SIGNAL(reportFinished(unsigned,bool)));
        m_thread->start();
    }
}

DatastoreController::~DatastoreController()
{
    if (m_haveDb) {
        emit sigQuit();
        m_thread->wait();
        m_worker->teardown();
    }

    delete m_worker;
}

void
DatastoreController::startSearch(const QString &str, int limit, unsigned &id)
{
    if (id != INVALID_SEARCH_ID)
        emit sigStopSearch(id);

    ++m_nextSearchId;
    m_nextSearchId += (m_nextSearchId == INVALID_SEARCH_ID);

    emit sigStartSearch(id = m_nextSearchId, limit, str);
}

void
DatastoreController::stopSearch(unsigned &id)
{
    if (id != INVALID_SEARCH_ID) {
        emit sigStopSearch(id);
        id = INVALID_SEARCH_ID;
    }
}

void
DatastoreController::storeCommand(const AttributeMap &commandInfo)
{
    const auto &command = commandInfo[g_attr_REGION_COMMAND];
    if (command.size() < DATASTORE_MINIMUM_LENGTH || command.contains('\n'))
        return;

    bool exitok;
    int exitcode = commandInfo.value(g_attr_REGION_EXITCODE).toInt(&exitok);

    if (exitok && exitcode >= 0 && exitcode != 127)
        emit sigStoreCommand(commandInfo);
}

void
DatastoreController::removeCommand(const QString &command)
{
    if (!command.isEmpty())
        emit sigRemoveCommand(command);
}

void
DatastoreController::storeAlias(const QString &alias, const QString &command)
{
    if (alias.size() >= 3 && alias.size() < command.size()) {
        AttributeMap aliasInfo;
        aliasInfo[g_ds_ALIAS] = alias;
        aliasInfo[g_ds_COMMAND] = command;
        aliasInfo[g_ds_UPDATED] = QString::number(osWalltime());

        emit sigStoreAlias(aliasInfo);
    }
}
