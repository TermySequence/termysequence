// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QCoreApplication;
QT_END_NAMESPACE

class IconApplication final: public QObject
{
    Q_OBJECT

private:
    QCoreApplication *m_app;
    int m_command;

private slots:
    void handleIconsLoaded();

public:
    IconApplication(QCoreApplication *app, int command);
    int exec();
};
