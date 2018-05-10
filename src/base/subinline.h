// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseinline.h"

class InlineSubarea final: public InlineBase
{
private:
    InlineBase *m_super;

    void setClicking(bool clicking);
    void unsetRenderFlags(uint8_t flags);

    QMenu* getPopup(MainWindow *window);
    void getDragData(QMimeData *data, QDrag *drag);
    void open();

public:
    InlineSubarea(TermWidget *parent);

    void setSubRegion(InlineBase *super, unsigned index);
    void setRegion(const Region *region);
    void bringDown();

    void setRenderFlags(uint8_t flags);
};
