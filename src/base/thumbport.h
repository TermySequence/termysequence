// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "viewport.h"
#include "region.h"

class TermScreen;

class TermThumbport final: public TermViewport
{
    Q_OBJECT

private:
    TermScreen *m_screen;
    RegionList m_regions;

    void doUpdateRegions();
    void updateRegions();

private slots:
    void handleScreenMoved(int offset);
    void handleSizeChanged(QSize size);

public:
    TermThumbport(TermInstance *term, QObject *parent);

    inline const RegionList& regions() const { return m_regions; }

    QPoint cursor() const;
};
