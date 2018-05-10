// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QLabel>

class EllipsizingLabel final: public QLabel
{
    Q_OBJECT

private:
    QStringList m_lines;

    void retext(int width);

protected:
    void resizeEvent(QResizeEvent *event);
    void changeEvent(QEvent *event);

public slots:
    void setText(const QString &text);

public:
    EllipsizingLabel(QWidget *parent = 0);
};
