// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE
struct FormatDef;

class FormatDialog final: public QDialog
{
    Q_OBJECT

private:
    QString m_saved;
    QString m_defval;

    QLineEdit *m_text;
    QComboBox *m_combo;

private:
    void populateCombo(const FormatDef *specs);

private slots:
    void handleInsert();
    void handleReset();

protected:
    bool event(QEvent *event);

public:
    FormatDialog(const FormatDef *specs, const QString &defval, QWidget *parent);

    QString text() const;
    void setText(const QString &text);
};
