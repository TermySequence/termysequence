// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QPlainTextEdit;
class QCheckBox;
class QLabel;
QT_END_NAMESPACE
class DircolorsView;

class DircolorsTab final: public QWidget
{
    Q_OBJECT

private:
    TermPalette &m_palette;

    DircolorsView *m_view;

    QPushButton *m_updateButton;
    QPlainTextEdit *m_text;
    QCheckBox *m_inherit;
    QLabel *m_status;

private slots:
    void handleUpdate();
    void handleTextChanged();
    void handleInherit(bool inherit);

    void handleSelect(int start, int end);
    void handleAppend(const QString &str);

signals:
    void modified();

public:
    DircolorsTab(TermPalette &palette, const QFont &font);

    void reload();
};
