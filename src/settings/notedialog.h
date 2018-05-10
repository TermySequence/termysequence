// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/region.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
class TermInstance;

class NoteDialog final: public QDialog
{
    Q_OBJECT

private:
    QLineEdit *m_char, *m_note;

    TermInstance *m_term;
    Region m_region;

private slots:
    void handleReset();
    void handleAccept();

protected:
    bool event(QEvent *event);

public:
    NoteDialog(TermInstance *term, const RegionBase *region, QWidget *parent);
};
