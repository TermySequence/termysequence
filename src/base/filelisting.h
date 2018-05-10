// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>
#include <QScrollArea>
#include <QVector>

struct TermFile;
class FileFilter;
class FileNameItem;
class FileWidget;
class FileScroll;

//
// List view
//
class FileListing final: public QWidget, public FontBase
{
    Q_OBJECT

private:
    FileFilter *m_filter;
    FileNameItem *m_nameitem;
    FileWidget *m_parent;
    FileScroll *m_scroll;

    bool m_visible = false;
    bool m_restart = false;
    bool m_clicking = false;

    QVector<std::pair<QRect,QRect>> m_list;

    int *m_widths = nullptr;
    int m_cols = 1;

    int m_selected = -1;
    int m_mouseover = -1;
    QPoint m_dragStartPosition;

    QFont m_font;
    QSize m_sizeHint{1, 1};

    QString m_savedSelection;

    bool calculateColumnWidths(int total, int cols);
    void positionRectangles(int total);
    void relayout(bool restart);

    int computeWidth(const TermFile *file);
    int getClickIndex(QPoint pos) const;
    void mouseAction(int action) const;

private slots:
    void refont(const QFont &font);

    void handleDataChanged(const QModelIndex &si, const QModelIndex &ei,
                           const QVector<int> &roles);
    void handleRowsInserted(const QModelIndex &parent, int start, int end);
    void handleRowsRemoved(const QModelIndex &parent, int start, int end);
    void handleLayoutChanging(const QList<QPersistentModelIndex> &parents);
    void handleLayoutChanged(const QList<QPersistentModelIndex> &parents);
    void handleModelReset();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    FileListing(FileFilter *filter, FileNameItem *item,
                FileWidget *parent, FileScroll *scroll);
    ~FileListing();

    void selectItem(int selected);

    QSize sizeHint() const;
};

//
// Scroll area
//
class FileScroll final: public QScrollArea
{
private:
    FileNameItem *m_nameitem;

protected:
    void scrollContentsBy(int dx, int dy);

public:
    FileScroll(FileNameItem *item);

    void setWidget(QWidget *widget);
};
