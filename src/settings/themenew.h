// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE
class ThemeSettings;

class NewThemeDialog final: public QDialog
{
    Q_OBJECT

private:
    QLineEdit *m_name;
    QComboBox *m_group;
    QCheckBox *m_lesser;

    ThemeSettings *m_from;

signals:
    void okayed();

public:
    NewThemeDialog(QWidget *parent, ThemeSettings *from = nullptr);
    ~NewThemeDialog();

    ThemeSettings* from() const { return m_from; }

    QString name() const;
    void setName(const QString &name);
    QString group() const;
    void setGroup(const QString &group);
    bool lesser() const;
    void setLesser(bool lesser);
};
