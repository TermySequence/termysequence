// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE
class IdBase;
class InfoAttrModel;

class InfoTab final: public QWidget
{
    Q_OBJECT

private:
    InfoAttrModel *m_model;

    QLineEdit *m_key, *m_value;
    QCheckBox *m_check;
    QPushButton *m_button;

private slots:
    void handleSelection(const QModelIndex &index);
    void handleCheckBox(bool checked);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

signals:
    void changeSubmitted();

public:
    InfoTab(IdBase *subject);

    QString key() const;
    QString value() const;
    bool removal() const;
};
