// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>
#include <QHash>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QRadioButton;
QT_END_NAMESPACE
class TermInstance;
class ServerInstance;
class TermManager;

class DropDialog final: public QDialog
{
    Q_OBJECT

private:
    QHash<QRadioButton*,int> m_choices;

    TermInstance *m_term;
    ServerInstance *m_server;
    TermManager *m_manager;

    QList<QUrl> m_urls;
    int m_action;

private slots:
    void handleAccept();

public:
    DropDialog(TermInstance *term, ServerInstance *server,
               int action, QList<QUrl> &urls, TermManager *manager);

    void start();
    void run();
};
