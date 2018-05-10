// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QLabel>

class TermManager;
class StatusFlags;
class StatusInfo;
class StatusProfile;

// Priorities for status text
#define STATUSPRIO_OWNERSHIP   1
#define STATUSPRIO_CONNECTION  2

class MainStatus final: public QWidget
{
    Q_OBJECT

private:
    QLabel *m_minor;
    QLabel *m_major;
    StatusFlags *m_flags;
    StatusInfo *m_info;
    StatusProfile *m_profile;

    QWidget *m_main;

    int m_minorId = 0, m_majorId = 0;

    QStringList m_statusText;
    QString m_permanentText;

    void relayout();

protected:
    void timerEvent(QTimerEvent *event);
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    MainStatus(TermManager *manager);
    void doPolish();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

public slots:
    void showMinor(const QString &text);
    void showMajor(const QString &text);
    void showStatus(const QString &text, int priority);
    void showPermanent(const QString &text);

    void clearMinor();
    void clearMajor();
    void clearStatus(int priority);
    void clearPermanent();
};
