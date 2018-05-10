// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class PortEditModel;
class PortEditView;
class PortEditor;

class PortsDialog final: public QDialog
{
    Q_OBJECT

private:
    PortEditModel *m_model;
    PortEditView *m_view;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_editButton;

    PortEditor *m_dialog = nullptr;

private slots:
    void handleSelect();

    void handleAdd();
    void handleRemove();
    void handleEdit();

protected:
    bool event(QEvent *event);

public:
    PortsDialog(QWidget *parent);

    inline PortEditModel* model() { return m_model; }

    QSize sizeHint() const { return QSize(800, 400); }

    void bringUp();
};
