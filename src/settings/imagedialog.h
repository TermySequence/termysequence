// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

class ImageModel;
class ImageView;
class IdBase;

class ImageDialog final: public QDialog
{
    Q_OBJECT

private:
    ImageModel *m_model;
    ImageView *m_view;

    const QString m_idStr;

    void setup(int iconType, bool resettable);

private slots:
    void handleTextChanged(const QString &text);
    void handleModelReset();
    void handleReset();
    void handleAccepted();

signals:
    void okayed(QString icon, QString id);

protected:
    bool event(QEvent *event);

public:
    ImageDialog(int iconType, bool resettable, QWidget *parent);
    ImageDialog(int iconType, IdBase *idbase, QWidget *parent);

    void setCurrentIcon(const QString &icon);

    QSize sizeHint() const { return QSize(640, 512); }
};
