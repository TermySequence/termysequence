// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QLineEdit>

class KeystrokeInput final: public QLineEdit
{
    Q_OBJECT

private slots:
    void handleTextChanged(const QString &text);

protected:
    bool eventFilter(QObject *object, QEvent *event);

signals:
    void keystrokeReceived(int key);

public:
    KeystrokeInput();
};
