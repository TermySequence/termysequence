// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/color.h"
#include "lib/uuid.h"
#include "lib/enums.h"

#include <QObject>
#include <QIcon>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE
class IdBase;
class TermInstance;
class ServerInstance;
class TermManager;
class InfoAnimation;
namespace Tsq { class ProtocolUnmarshaler; }

class TermTask: public QObject
{
    Q_OBJECT

private:
    const bool m_longRunning;
    bool m_noTarget = false;
    bool m_started = false;
    bool m_finished = false;
    bool m_succeeded = false;
    bool m_questioning = false;
    bool m_questionCancel = false;
    bool m_noStatusPopup = false;

    char m_progress = -2;
    char m_questionType;

    int m_timerId = 0;
    ColorName m_finishColor = ErrorFg;

    QDateTime m_startedDate;
    QDateTime m_finishedDate;
    QString m_startedStr;
    QString m_finishedStr;
    QString m_statusStr;
    QString m_questionStr;

    InfoAnimation *m_animation;
    QObject *m_dialog = nullptr;

    Tsq::Uuid m_serverId;
    Tsq::Uuid m_termId;

    QMetaObject::Connection m_mocTask;

    void setTargetStr();
    void stop(const QString &msg, ColorName color, bool destroyed = false);

    bool copyAsText(const QByteArray &data);
    bool copyAsImage(const QByteArray &data);

protected:
    const Tsq::Uuid m_taskId{true};
    const QString m_taskIdStr;

    IdBase *m_target;
    ServerInstance *m_server;
    TermInstance *m_term;
    inline void setNoTarget() { m_noTarget = true; }

    QString m_typeStr;
    QString m_fromStr;
    QString m_toStr;
    QString m_sourceStr;
    QString m_sinkStr;
    QIcon m_typeIcon;

    size_t m_sent = 0, m_received = 0;

    void setStatus(const QString &msg, int progress = 0);
    void setProgress(size_t numer, size_t denom);
    void setQuestion(Tsq::TaskQuestion type, const QString &msg);
    void clearQuestion();

    void failStart(TermManager *manager, const QString &msg);
    bool doStart(TermManager *manager);
    void finish();
    void fail(const QString &msg);
    void launch(const QString &file);
    void finishCopy(const QByteArray &data, const QString &format = QString());

    void queueTaskChange();
    void timerEvent(QTimerEvent *event);

private slots:
    void handleTargetDestroyed();
    void handleTargetDisconnected();

protected slots:
    virtual void handleThrottle(bool throttled);

signals:
    void taskChanged();
    void taskQuestion();
    void ready(QString file);

public:
    TermTask(bool longRunning);
    TermTask(TermInstance *term, bool longRunning);
    TermTask(ServerInstance *server, bool longRunning);

    inline void disableStatusPopup() { m_noStatusPopup = true; }

    inline const Tsq::Uuid& id() const { return m_taskId; }
    inline const QString& idStr() const { return m_taskIdStr; }

    inline bool longRunning() const { return m_longRunning; }
    inline bool started() const { return m_started; }
    inline bool finished() const { return m_finished; }
    inline bool succeeded() const { return m_succeeded; }
    inline bool questioning() const { return m_questioning; }
    inline bool questionCancel() const { return m_questionCancel; }
    inline bool noStatusPopup() const { return m_noStatusPopup; }
    inline int progress() const { return m_progress; }
    inline int questionType() const { return m_questionType; }
    inline ColorName finishColor() const { return m_finishColor; }

    inline IdBase* target() { return m_target; }
    inline InfoAnimation* animation() const { return m_animation; }
    inline QObject* dialog() const { return m_dialog; }
    void setDialog(QObject *dialog);

    inline const QDateTime& startedDate() const { return m_startedDate; }
    inline const QDateTime& finishedDate() const { return m_finishedDate; }
    inline const QString& startedStr() const { return m_startedStr; }
    inline const QString& finishedStr() const { return m_finishedStr; }
    inline const QString& statusStr() const { return m_statusStr; }
    inline const QString& questionStr() const { return m_questionStr; }
    inline const QString& typeStr() const { return m_typeStr; }
    inline const QString& fromStr() const { return m_fromStr; }
    inline const QString& toStr() const { return m_toStr; }
    inline const QString& sourceStr() const { return m_sourceStr; }
    inline const QString& sinkStr() const { return m_sinkStr; }
    inline const QIcon& typeIcon() const { return m_typeIcon; }

    inline size_t sent() const { return m_sent; }
    inline size_t received() const { return m_received; }

    inline ServerInstance* server() { return m_server; }
    inline TermInstance* term() { return m_term; }
    inline const auto& serverId() const { return m_serverId; }
    inline const auto& termId() const { return m_termId; }

    virtual QString launchfile() const;
    virtual void getDragData(QMimeData *data) const;
    virtual bool clonable() const;
    virtual TermTask *clone() const;

    virtual void start(TermManager *manager) = 0;
    virtual void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    virtual void handleQuestion(int question);
    virtual void handleAnswer(int answer);
    virtual void handleDisconnect();
    virtual void cancel() = 0;
};

inline void
TermTask::queueTaskChange()
{
    if (m_timerId == 0)
        m_timerId = startTimer(250);
}
