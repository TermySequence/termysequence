// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "thumbbase.h"

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE
class TermInstance;
class EllipsizingLabel;
class ThumbFrame;
class ThumbIcon;

class ThumbArrange final: public ThumbBase
{
    Q_OBJECT

private:
    TermInstance *m_term;
    ThumbFrame *m_thumb;

    ThumbIcon *m_icon;
    ThumbIcon *m_lock;
    ThumbIcon *m_owner;
    EllipsizingLabel *m_caption;

    QRect m_outerBounds;
    QRect m_thumbBounds;
    QRect m_iconBounds;
    QRect m_captionBounds;
    QRect m_lockBounds;
    QRect m_ownerBounds;
    bool m_haveCaption;

    bool m_showIcon;
    bool m_showIndicators;

    void calculateBounds();
    void mouseAction(int index) const;

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private slots:
    void refocus();
    void relayout();
    void handleFormatChanged(const QString &format);
    void handleIconSettings();
    void handleIndicators();

public:
    ThumbArrange(TermInstance *term, TermManager *manager, BarWidget *parent);

    inline TermInstance* term() { return m_term; }
    bool hidden() const;

    void clearDrag();
    bool setNormalDrag(const QMimeData *data);
    void dropNormalDrag(const QMimeData *data);

    int heightForWidth(int w) const;
    int widthForHeight(int h) const;
};
