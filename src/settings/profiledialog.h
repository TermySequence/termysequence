// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE
class IdBase;

//
// Choose profile
//
class ProfileDialog final: public QDialog
{
    Q_OBJECT

private:
    QString m_idStr;
    QComboBox *m_combo;

private slots:
    void handleAccepted();
    void handleManage();

signals:
    void okayed(QString profileName, QString id);

public:
    ProfileDialog(IdBase *idbase, QWidget *parent);
};

//
// Choose launcher
//
class LauncherDialog final: public QDialog
{
    Q_OBJECT

private:
    QString m_idStr, m_uri, m_subs;
    QComboBox *m_combo;

private slots:
    void handleAccepted();
    void handleManage();

signals:
    void okayed(QString choice, QString id, QString uri, QString subs);

public:
    LauncherDialog(QWidget *parent, IdBase *idbase, const QString &uri,
                   const QString &subs = QString());
};

//
// Choose alert
//
class AlertDialog final: public QDialog
{
    Q_OBJECT

private:
    QString m_idStr;
    QComboBox *m_combo;

private slots:
    void handleAccepted();
    void handleManage();

signals:
    void okayed(QString choice, QString id);

public:
    AlertDialog(IdBase *idbase, QWidget *parent);
};

//
// Choose slot
//
class SlotDialog final: public QDialog
{
    Q_OBJECT

private:
    QStringList m_history;
    QComboBox *m_combo;

private slots:
    void handleAccepted();
    void handleClear();

signals:
    void okayed(QString slot, bool fromKeyboard);

public:
    SlotDialog(QWidget *parent);

    QSize sizeHint() const;
};
