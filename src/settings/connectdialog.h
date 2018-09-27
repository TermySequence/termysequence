// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QCheckBox;
class QDialogButtonBox;
class QVBoxLayout;
QT_END_NAMESPACE
class ConnectSettings;
class ServerCombo;

class ConnectDialog: public QDialog
{
    Q_OBJECT

protected:
    ConnectSettings *m_info = nullptr;

    QVBoxLayout *m_mainLayout;
    QLineEdit *m_saveName;
    QPushButton *m_okButton;
    QWidget *m_focusWidget = nullptr;
    ServerCombo *m_serverCombo;

    void setSaveName(const QString &text);

    void createInfo(const QString &defaultName);
    void doSave(const QString &name);
    void doAccept();

private:
    QDialogButtonBox *m_buttonBox;
    QCheckBox *m_save = nullptr;

private slots:
    virtual void handleAccept() = 0;

protected:
    bool event(QEvent *event);

signals:
    void saved(ConnectSettings *conn);

public:
    enum Options { ShowServ = 1, OptSave = 2, ReqSave = 4 };

    ConnectDialog(QWidget *parent, const char *helpPage, unsigned options);

    inline ConnectSettings* conn() { return m_info; }
};
