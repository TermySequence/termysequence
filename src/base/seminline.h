// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseinline.h"

class InlineSemantic: public InlineBase
{
private:
    QString m_icon;

    void handleHighlight(regionid_t regionid);

protected:
    // Used By InlineLink
    InlineSemantic(Tsqt::RegionType type, TermWidget *parent);
    void setToolTip(const QString &link);

    QMenu* getPopup(MainWindow *window);
    void getDragData(QMimeData *data, QDrag *drag);
    void open();

public:
    InlineSemantic(TermWidget *parent);

    void setRegion(const Region *region);
};
