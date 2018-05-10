// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
QT_END_NAMESPACE
struct SettingDef;
class SettingsBase;

class CommandDialog final: public QDialog
{
    Q_OBJECT

private:
    const SettingDef *m_def;
    SettingsBase *m_settings;

    QLineEdit *m_prog;
    QListWidget *m_view;

    QPushButton *m_insertButton;
    QPushButton *m_prependButton;
    QPushButton *m_appendButton;
    QPushButton *m_deleteButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_resetButton;

    QStringList m_saved;

private slots:
    void handleSelection();
    void handleChange();

    void handleInsertArg();
    void handlePrependArg();
    void handleAppendArg();
    void handleDeleteArg();
    void handleUp();
    void handleDown();

    void handleTextEdited(const QString &text);
    void handleReset();
    void handleAccept();
    void handleSh();

protected:
    bool event(QEvent *event);

public:
    CommandDialog(const SettingDef *def, SettingsBase *settings, QWidget *parent);

    void setContent(QStringList list);
};
