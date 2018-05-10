// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QScrollArea>
#include <QHash>

class BarScroll;
class TermManager;
class TermInstance;
class ServerInstance;
class ThumbBase;
class ThumbArrange;
class ServerArrange;

//
// Contents Widget
//
class BarWidget final: public QWidget
{
    Q_OBJECT

private:
    TermManager *m_manager;
    BarScroll *m_parent;

    QHash<QObject*,ThumbBase*> m_widgetMap;

    QSize m_sizeLimit;
    QSize m_sizeHint;

    bool m_horizontal = true;
    bool m_dense;

    bool m_reorderDrag;
    bool m_reorderAfter;
    bool m_normalDragOk;
    QString m_dragTargetId;
    ThumbBase *m_dragTargets[2]{};
    QByteArray m_dragSource;

    int m_dragDirection = 0;
    int m_timerId = 0;

    void placeHorizontal(ThumbBase *widget, QRect &bounds, int height);
    void placeVertical(ThumbBase *widget, QRect &bounds, int width);

    ThumbBase* findDragTarget(QPoint dragPos);
    void stopDrag();
    void updateDragAutoScroll(QPoint dragPos);

    ThumbBase* updateNormalDrag(QDropEvent *event);
    ThumbBase* updateReorderDrag(QDropEvent *event);
    void updateReorderDragHelper(QPoint dragPos, ThumbBase *dragTarget);

private slots:
    void handleServerAdded(ServerInstance *server);
    void handleServerRemoved(ServerInstance *server);
    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);
    void handleTermActivated(TermInstance *term);
    void handleSettingsChanged();

protected:
    void timerEvent(QTimerEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    bool event(QEvent *event);

public:
    BarWidget(TermManager *manager, BarScroll *parent, const QSize &limit);

    inline bool horizontal() const { return m_horizontal; }

    void relayoutHorizontal();
    void relayoutVertical();

    QSize sizeHint() const;

public slots:
    void relayout();
};

//
// Scroll Area
//
class BarScroll final: public QScrollArea
{
private:
    BarWidget *m_widget;

    QSize m_sizeHint;

protected:
    void resizeEvent(QResizeEvent *event);

public:
    BarScroll(TermManager *manager);

    void scroll(int direction, bool horizontal);

    QSize sizeHint() const;
};
