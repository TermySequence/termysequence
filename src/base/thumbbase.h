// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE
class IdBase;
class TermManager;
class BarWidget;
class DragIcon;

class ThumbBase: public QWidget
{
    Q_OBJECT

protected:
    TermManager *m_manager;
    BarWidget *m_parent;
    QString m_targetIdStr;

    bool m_hover = false;
    bool m_reorderingBefore = false;
    bool m_reorderingAfter = false;
    bool m_normalDrag = false;
    bool m_acceptDrag;

    enum ColorName {
        NormalBg = -1, HoverBg, NormalFg, ActiveBg, ActiveHoverBg, ActiveFg,
        InactiveBg, InactiveHoverBg, InactiveFg, NColors
    };
    ColorName m_bgName = NormalBg;
    ColorName m_fgName = NormalBg; // not a typo
    QColor m_colors[NColors];

    void calculateColors(bool lightenActive);
    void paintCommon(QPainter *painter);

private:
    DragIcon *m_drag = nullptr;
    QRect m_dragBounds;

    QPoint m_dragStartPosition;
    bool m_clicking = false;

    void paintTop(QPainter *painter);
    void paintBottom(QPainter *painter);
    void paintLeft(QPainter *painter);
    void paintRight(QPainter *painter);

    virtual void mouseAction(int index) const = 0;

protected:
    void resizeEvent(QResizeEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    ThumbBase(IdBase *target, TermManager *manager, BarWidget *parent);

    inline const QString& idStr() const { return m_targetIdStr; }
    virtual bool hidden() const;

    virtual int heightForWidth(int w) const = 0;
    virtual int widthForHeight(int h) const = 0;

    virtual void clearDrag();
    void setReorderDrag(bool before);
    virtual bool setNormalDrag(const QMimeData *data);
    virtual void dropNormalDrag(const QMimeData *data) = 0;
};
