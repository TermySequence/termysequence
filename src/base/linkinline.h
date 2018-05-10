// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "seminline.h"

class InlineLink final: public InlineSemantic
{
private:
    bool m_isfile;
    bool m_issem;

    QMenu* getPopup(MainWindow *window);
    void getDragData(QMimeData *data, QDrag *drag);
    void open();

    void handleHighlight(regionid_t regionid);

public:
    InlineLink(TermWidget *parent);

    void setRegion(const Region *region);
};
