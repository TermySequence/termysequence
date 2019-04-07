// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE
struct SettingDef;
class SettingsBase;

class EncodingDialog final: public QDialog
{
    Q_OBJECT

private:
    const SettingDef *m_def;
    SettingsBase *m_settings;

    QPlainTextEdit *m_params;
    QString m_variant;

private slots:
    void handleAccept();

protected:
    bool event(QEvent *event);

public:
    EncodingDialog(const SettingDef *def, SettingsBase *settings,
                   QWidget *parent);

    void setContent(const QStringList &params);

    QSize sizeHint() const { return QSize(640, 480); }
};
