// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLineEdit;
class QTabWidget;
QT_END_NAMESPACE
struct SettingDef;
class SettingsBase;

class EnvironDialog final: public QDialog
{
    Q_OBJECT

private:
    const SettingDef *m_def;
    SettingsBase *m_settings;

    QPlainTextEdit *m_set, *m_clear;
    QLineEdit *m_answerback = nullptr;
    QTabWidget *m_tabs;

private slots:
    void handleAccept();
    void handleDefaults();

protected:
    bool event(QEvent *event);

public:
    EnvironDialog(const SettingDef *def, SettingsBase *settings,
                  bool answerback, QWidget *parent);

    void setContent(const QStringList &rules);

    QSize sizeHint() const { return QSize(640, 480); }
};
