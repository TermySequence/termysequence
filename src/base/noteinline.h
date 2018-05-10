// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseinline.h"

class InlineNote final: public InlineBase
{
private:
    QMenu* getPopup(MainWindow *window);
    void getDragData(QMimeData *data, QDrag *drag);
    void open();

public:
    InlineNote(TermWidget *parent);

    void setRegion(const Region *region);
};
