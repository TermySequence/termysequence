// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "imageinline.h"
#include "termwidget.h"
#include "term.h"
#include "thumbicon.h"
#include "contentmodel.h"
#include "infowindow.h"
#include "mainwindow.h"
#include "scrollport.h"
#include "manager.h"

#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

InlineImage::InlineImage(TermWidget *parent) :
    InlineBase(Tsqt::RegionImage, parent),
    m_icon(L("image"))
{
    m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, m_icon);
    connect(m_term->content(), SIGNAL(contentRemoved(TermContent*)),
            SLOT(handleContentRemoved(TermContent*)));
    connect(m_term->content(), SIGNAL(contentFetching(TermContent*)),
            SLOT(handleContentFetching(TermContent*)));
}

InlineImage::~InlineImage()
{
    if (m_movie)
        m_term->content()->stopMovie(m_content);
}

void
InlineImage::handleContentRemoved(TermContent *content)
{
    if (m_content == content) {
        m_content = nullptr;
        m_movie = nullptr;
    }
}

void
InlineImage::handleContentFetching(TermContent *content)
{
    if (m_content == content)
        m_needContent = true;
}

void
InlineImage::handleMovieUpdate()
{
    m_pixmap = m_movie->currentPixmap();
    update();
}

inline void
InlineImage::stopMovie()
{
    if (m_movie) {
        m_movie->disconnect(this);
        m_term->content()->stopMovie(m_content);
        m_movie = nullptr;
    }
}

void
InlineImage::setContent()
{
    if (!m_content->enabled)
        return;

    m_pixmap = m_content->pixmap;
    m_renderMask &= ~AlwaysOn;

    if (m_content->movie) {
        m_movie = &m_content->movie->movie;
        connect(m_movie, SIGNAL(updated(const QRect&)), SLOT(handleMovieUpdate()));
        m_term->content()->startMovie(m_content);
    }
    if (m_keepAspect) {
        int y = 0, width = m_w, height = m_h;

        QSize size(m_pixmap.size());
        size.scale(width, height, Qt::KeepAspectRatio);

        if (height > size.height())
            y = (height - size.height()) / 2;

        width = size.width();
        height = size.height();
        setPixelBounds(y, width, height);
    }
}

void
InlineImage::bringDown()
{
    stopMovie();
    m_content = nullptr;
    InlineBase::bringDown();
}

void
InlineImage::bringUp()
{
    if (m_needContent) {
        m_content = m_term->content()->content(m_contentId);
        if (m_content && m_content->loaded) {
            setContent();
            m_needContent = false;
        }
    }

    InlineBase::bringUp();
}

void
InlineImage::setRegion(const Region *region)
{
    stopMovie();

    m_region = region;
    m_pixmap = QPixmap();
    m_needContent = false;
    m_renderMask |= AlwaysOn;
    QString icon;

    QString uri = region->attributes.value(g_attr_CONTENT_NAME);
    setToolTip(uri);
    setUrl(TermUrl::parse(uri));

    m_inline = region->attributes.value(g_attr_CONTENT_INLINE) == A("1");
    if (!m_inline) {
        icon = L("content");
        m_keepAspect = false;
        m_content = nullptr;
        setBounds(region->startCol - 1, 0, 2, 1);
    }
    else {
        icon = L("image");
        m_keepAspect = region->attributes.value(g_attr_CONTENT_ASPECT) != A("0");
        m_contentId = region->attributes.value(g_attr_CONTENT_ID);
        m_content = m_term->content()->content(m_contentId);

        setBounds(region->startCol, 0, region->endCol - region->startCol,
                  region->endRow - region->startRow + 1);

        if (!m_content || !m_content->loaded)
            m_needContent = true;
        else
            setContent();
    }

    if (m_icon != icon) {
        m_icon = icon;
        m_renderer = ThumbIcon::getRenderer(ThumbIcon::SemanticType, icon);
    }
}

void
InlineImage::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    paintPart1(painter);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(rect(), m_pixmap);

    paintPart2(painter);
}

void
InlineImage::open()
{
    // no setExecuting()
    QString id = m_region->attributes.value(g_attr_CONTENT_ID);
    InfoWindow::showContentWindow(m_term, id);
}

QMenu *
InlineImage::getPopup(MainWindow *window)
{
    TermContentPopup params;
    params.id = m_region->attributes.value(g_attr_CONTENT_ID);
    params.tu = m_url;
    params.isimage = m_inline;
    params.isshown = (m_content && m_content->enabled) || !m_inline;
    params.addTermMenu = true;

    return window->getImagePopup(m_term, &params);
}

void
InlineImage::getDragData(QMimeData *data, QDrag *drag)
{
    QSizeF size = m_parent->cellSize() * 5;
    data->setImageData(m_pixmap.toImage());
    drag->setPixmap(m_pixmap.scaled(size.toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void
InlineImage::clipboardCopy()
{
    QApplication::clipboard()->setImage(m_pixmap.toImage());
    m_scrollport->manager()->reportClipboardCopy(0, CopiedImage);
}

void
InlineImage::textSelect()
{}

void
InlineImage::textWrite()
{}
