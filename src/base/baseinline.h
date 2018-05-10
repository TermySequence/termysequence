// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "region.h"
#include "url.h"

#include <QWidget>
#include <QRectF>
#include <QVector>
#include <QSvgRenderer>
#include <map>
#include <unordered_set>

QT_BEGIN_NAMESPACE
class QMenu;
class QPropertyAnimation;
class QMimeData;
class QDrag;
QT_END_NAMESPACE
class TermWidget;
class TermInstance;
class TermScrollport;
class MainWindow;
class InlineSubarea;

//
// Base Class
//
class InlineBase: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int highlightFade READ highlightFade WRITE setHighlightFade)
    Q_PROPERTY(int actionFade READ actionFade WRITE setActionFade)

    friend class InlineSubarea;
    friend class InlineCache;

private:
    const Tsqt::RegionType m_type;

protected:
    const Region *m_region;
    TermWidget *m_parent;
    TermInstance *m_term;
    TermScrollport *m_scrollport;
    QSvgRenderer *m_renderer = nullptr;
    TermUrl m_url;

    QVector<QRect> m_subrects;
    QVector<InlineSubarea*> m_subareas;

    enum RenderMask: uint8_t {
        Highlighting = 1, Hovering = 2, Menuing = 4, Selected = 8,
        AlwaysOn = 128
    };
    uint8_t m_renderMask = 0, m_borderMask = 0;
    int m_x, m_y, m_w, m_h;

private:
    bool m_visible = false;
    bool m_highlight = false;
    bool m_action = false;
    bool m_clicking = false;
    bool m_selected = false;

    qreal m_cw, m_ch;
    QRectF m_border;
    QRectF m_icon;

    int m_highlightFade;
    int m_highlightAlpha = 255;
    QPropertyAnimation *m_highlightAnim;
    int m_line;
    int m_actionFade;
    QPropertyAnimation *m_actionAnim;

    QPoint m_dragStartPosition;

    QMenu *m_menu = nullptr;

    QMetaObject::Connection m_mocHighlight;
    QMetaObject::Connection m_mocMenu;

    virtual QMenu* getPopup(MainWindow *window) = 0;
    virtual void getDragData(QMimeData *data, QDrag *drag) = 0;
    virtual void open() = 0;
    virtual void clipboardCopy();
    virtual void textSelect();
    virtual void textWrite();

    void mouseAction(QMouseEvent *event, int action);

protected:
    virtual void setClicking(bool clicking);
    void setSelected(bool selected);
    void setExecuting();
    void setUrl(const TermUrl &url);

    void setBounds(int x, int y, int w, int h);
    void setPixelBounds(int y, int w, int h);
    void moveIcon();

    void paintPart1(QPainter &painter);
    void paintPart2(QPainter &painter);

protected slots:
    virtual void handleHighlight(regionid_t regionid);

private slots:
    virtual void setRenderFlags(uint8_t flags);
    virtual void unsetRenderFlags(uint8_t flags);

    void handleHighlightFinished();
    void handleMenuFinished();

protected:
    void paintEvent(QPaintEvent *event);
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

public:
    InlineBase(Tsqt::RegionType type, TermWidget *parent);

    inline auto type() const { return m_type; }

    virtual void setRegion(const Region *region) = 0;
    virtual void bringDown();
    virtual void bringUp();
    void checkUrl(const TermUrl &url);

    void setHighlighting(unsigned duration = 5);
    inline int highlightFade() const { return m_highlightFade; }
    void setHighlightFade(int highlightFade);
    inline int actionFade() const { return m_actionFade; }
    void setActionFade(int actionFade);

    QMenu* getInlinePopup(MainWindow *window);

    void calculateCellSize();
};

//
// Widget Cache
//
class InlineCache
{
private:
    TermWidget *m_parent;

    typedef std::pair<Tsqt::RegionType,regionid_t> RegionKey;
    typedef std::map<RegionKey,InlineBase*> InlineMap;

    InlineMap m_active;
    std::unordered_set<InlineBase*> m_inactive;
    std::unordered_set<InlineSubarea*> m_inactiveSubs;

    void setupWidget(const Region *region, InlineBase *base);
    void getWidget(const Region *region, InlineMap &addto);

public:
    InlineCache(TermWidget *parent);

    void process(const RegionList &regions);
    void updateUrl(const TermUrl &url);
    void calculateCellSize();
};
