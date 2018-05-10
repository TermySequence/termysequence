// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "port.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QComboBox;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE
class ServerInstance;

class PortEditor final: public QDialog
{
    Q_OBJECT

private:
    QRadioButton *m_local, *m_remote;
    QComboBox *m_ltype, *m_ctype;
    QLineEdit *m_laddr, *m_caddr;
    QLineEdit *m_lport, *m_cport;

    QLabel *m_label;
    ServerInstance *m_server;

private slots:
    void handleTypeChange();
    void handleAccept();
    void handleLink();

public:
    PortEditor(QWidget *parent, bool showlink);

    void setAdding();
    void setServer(ServerInstance *server);

    PortFwdRule buildRule() const;
    void setRule(const PortFwdRule &rule);
};
