// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "thumbbase.h"
#include "fontbase.h"

#include <QPainterPath>

QT_BEGIN_NAMESPACE
class QLabel;
class QMimeData;
QT_END_NAMESPACE
class ServerInstance;
class EllipsizingLabel;
class ThumbIcon;

class ServerArrange final: public ThumbBase, public FontBase
{
    Q_OBJECT

private:
    ServerInstance *m_server;

    ThumbIcon *m_icon;
    ThumbIcon *m_disconnect;
    EllipsizingLabel *m_caption;
    QLabel *m_count;

    QRect m_iconBounds;
    QRect m_captionBounds;
    QRect m_countBounds;

    QPainterPath m_path;

    void calculateBounds();
    void relayout();
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

    void handleTermsChanged(ServerInstance *server, size_t total, size_t hidden);
    void handleConnection();

public:
    ServerArrange(ServerInstance *server, TermManager *manager, BarWidget *parent);

    void clearDrag();
    bool setNormalDrag(const QMimeData *data);
    void dropNormalDrag(const QMimeData *data);

    int heightForWidth(int w) const;
    int widthForHeight(int h) const;
};
