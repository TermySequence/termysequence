// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "adjustdialog.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE
class FontPreview;

class FontDialog final: public AdjustDialog
{
    Q_OBJECT

private:
    QSpinBox *m_spin;
    FontPreview *m_preview;

    QFont m_font, m_savedFont;

    void handleFontResult(const QFont &font);

private slots:
    void handleFontSize(int size);
    void handleFontSelect();

    void handleAccept();
    void handleRejected();
    void handleReset();

public:
    FontDialog(TermInstance *term, TermManager *manager, QWidget *parent);
};
