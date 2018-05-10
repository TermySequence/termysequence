// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QStandardItemModel;
QT_END_NAMESPACE

class SetupDialog final: public QDialog
{
    Q_OBJECT

private:
    QLineEdit *m_repo;
    QLineEdit *m_inst;
    QStandardItemModel *m_sources;

private slots:
    void handleAccept();

public:
    SetupDialog(QWidget *parent = nullptr);

    QSize sizeHint() const { return QSize(800, 600); }
};
