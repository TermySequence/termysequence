// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/flags.h"
#include "statuslabel.h"

#include <QVector>

class MainStatus;
class TermManager;
class TermInstance;
class TermScrollport;

struct FlagString
{
    Tsq::TermFlags flag;
    QString str;
    QString tooltip;
};

class StatusFlags final: public StatusLabel
{
    Q_OBJECT

private:
    MainStatus *m_parent;
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport;
    TermManager *m_manager;

    QMetaObject::Connection m_mocFlags;

    QVector<std::pair<QString,QString>> m_strings;

    QVector<FlagString> m_flagspecs;
    Tsq::TermFlags m_mask = 0, m_flags = 0;
    QString m_followspec, m_leaderspec, m_throttlespec;
    QString m_rawTip, m_followTip, m_holdTip;

    void handleOwnership();
    void updateText(int index, const QString &str, const QString &tip);

private slots:
    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);

    void handleFlagsChanged(Tsq::TermFlags flags);
    void handleAttributeChanged(const QString &key, const QString &value);
    void handleProcessChanged(const QString &key);
    void handleInputChanged();
    void handleLockedChanged();
    void handleThrottleChanged(bool throttled);

    void handleClicked();

public:
    StatusFlags(TermManager *manager, MainStatus *parent);
    void doPolish();
};
