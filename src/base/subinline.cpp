// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "subinline.h"

InlineSubarea::InlineSubarea(TermWidget *parent) :
    InlineBase(Tsqt::RegionInvalid, parent)
{
}

void
InlineSubarea::setSubRegion(InlineBase *super, unsigned index)
{
    m_super = super;
    m_region = super->m_region;
    setUrl(super->m_url);
    setToolTip(super->toolTip());

    const auto &rect = super->m_subrects[index];
    setBounds(rect.x(), rect.y(), rect.width(), rect.height());
}

void
InlineSubarea::setRegion(const Region *)
{
}

void
InlineSubarea::bringDown()
{
    setVisible(false);
    m_selected = false;
    m_renderMask = 0;
    m_borderMask = 0;
}

void
InlineSubarea::setClicking(bool clicking)
{
    m_super->setClicking(clicking);
}

void
InlineSubarea::setRenderFlags(uint8_t flags)
{
    m_super->setRenderFlags(flags);
}

void
InlineSubarea::unsetRenderFlags(uint8_t flags)
{
    m_super->unsetRenderFlags(flags);
}

void
InlineSubarea::open()
{
    m_super->open();
}

QMenu *
InlineSubarea::getPopup(MainWindow *window)
{
    return m_super->getPopup(window);
}

void
InlineSubarea::getDragData(QMimeData *data, QDrag *drag)
{
    m_super->getDragData(data, drag);
}
