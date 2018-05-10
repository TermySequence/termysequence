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

    void setSaveName(const QString &text);

    void createInfo(const QString &defaultName);
    void doSave(const QString &name);
    void doAccept();

private:
    QDialogButtonBox *m_buttonBox;
    ServerCombo *m_serverCombo;
    QCheckBox *m_save;

private slots:
    virtual void handleAccept() = 0;

protected:
    bool event(QEvent *event);

signals:
    void saved(ConnectSettings *conn);

public:
    ConnectDialog(QWidget *parent, const char *helpPage,
                  bool showServ = true, bool showSave = true);
    void setNameRequired();

    inline ConnectSettings* conn() { return m_info; }
};
