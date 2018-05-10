// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QStackedWidget>

class MainWindow;
class StatusProgress;
class TermTask;

class StatusLink final: public QStackedWidget
{
    Q_OBJECT

private:
    MainWindow *m_parent;
    StatusProgress *m_progress;
    TermTask *m_task = nullptr;

    QMetaObject::Connection m_mocTask;

    QSize m_sizeHint;

private slots:
    void handleTaskChanged();
    void handleActiveTaskChanged(TermTask *task);

    void handleTaskClicked();

public:
    StatusLink(MainWindow *parent);

    QSize sizeHint() const;
};
