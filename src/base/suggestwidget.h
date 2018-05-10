// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "dockwidget.h"
#include "fontbase.h"

#include <QRgb>
#include <QVector>
#include <QSet>
#include <QHash>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE
class TermInstance;

struct SuggestInfo {
    int start, end;
    AttributeMap map;
};

class SuggestWidget final: public ToolWidget, public DisplayIterator
{
    Q_OBJECT

private:
    QVector<SuggestInfo> m_info;
    QSet<QString> m_commands;

    QString m_searchStr;
    unsigned m_searchId;
    QRgb m_matchBg, m_matchFg;
    bool m_first = true;

    int m_col = 0, m_row = 0, m_pos = 0;
    int m_cols = 0, m_rows = 0, m_size = 0;
    bool m_haveRoom = false;
    int m_activeIndex = -1;
    int m_hoverIndex = -1;
    int m_timerId = 0;

    QHash<int,QMenu*> m_popups;

    QSize m_sizeHint;

    void relayout();
    void setSearchString(const QString &searchStr);
    void clearSearchString();
    void stopRaiseTimer();
    void setRaiseTimer(bool delay);
    void stop();
    void clear();

    void addString(DisplayCell &params);
    DisplayCellList getCommandString(const DisplayCell &params,
                                     const QString &commandStr,
                                     const QString &acronymStr) const;

    void setActiveIndex(int index);
    int getClickIndex(const QPoint &pos) const;
    void action(int type, int index);

    bool m_clicking = false;
    QPoint m_dragStartPosition;
    void mouseAction(int action);

private slots:
    void handleSearchResult(unsigned id, AttributeMap map);
    void handleSearchFinished(unsigned id, bool overlimit);

    void handleTermActivated(TermInstance *term);
    void handleAttributeChanged(const QString &key, const QString &value);
    void handleAttributeRemoved(const QString &key);

    void refont(const QFont &font);
    void recolor();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    void timerEvent(QTimerEvent *event);

public:
    SuggestWidget(TermManager *manager);

    const QString& getCommand(int index);
    void removeCommand(int index);

    void toolAction(int index);
    void contextMenu();

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    QString getKillSequence() const;

    QSize sizeHint() const;
};
