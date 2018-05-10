// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>
#include <QVector>
#include <QVariant>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

class NewConnectDialog final: public QDialog
{
    Q_OBJECT

private:
    QComboBox *m_batch;
    QComboBox *m_type;

    struct TypeEntry {
        QIcon icon;
        QString name;
        QVariant value;
    };
    QVector<TypeEntry> m_types;

private slots:
    void handleBatchChanged(int index);

public:
    NewConnectDialog(QWidget *parent);

    int type() const;
};
