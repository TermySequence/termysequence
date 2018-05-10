// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>
#include <QVector>

class FileWidget;
class FileModel;
class FileNameItem;
class FileBanner;
struct TermDirectory;

//
// Path display
//
class FileBannerPath final: public QWidget, public DisplayIterator
{
    Q_OBJECT

private:
    FileModel *m_model;
    FileNameItem *m_nameitem;
    FileWidget *m_parent;

    QString m_part1, m_part2, m_path;
    QStringList m_names;
    QVector<QRect> m_namerects;
    QRect m_selrect;
    int m_margin = 1;

    bool m_visible = false, m_refresh = false;

    QRgb m_fg = 0;
    QRgb m_blend1, m_blend2;
    QSize m_sizeHint;

    int m_mouseover = -1;
    int m_clicked = -1;
    bool m_clicking = false;
    QPoint m_dragStartPosition;

    void relayout(bool refresh);

    int getClickIndex(QPoint pos) const;
    void mouseAction(int action) const;

private slots:
    void recolor(QRgb bg, QRgb fg);
    void refont(const QFont &font);

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
    FileBannerPath(FileBanner *parent);

    inline void setTerm(TermInstance *term) { m_term = term; }
    void setPath(const QString &path1, const QString &path2);

    QSize sizeHint() const;
};

//
// Arrangement
//
class FileBanner final: public QWidget, public DisplayIterator
{
    Q_OBJECT

    friend class FileBannerPath;

private:
    FileModel *m_model;
    FileNameItem *m_nameitem;
    FileWidget *m_parent;

    FileBannerPath *m_path;

    QString m_counts, m_notincwd, m_git;
    const QString *m_info, *m_infod;
    QRect m_inforect, m_gitrect;
    int m_margin = 1;

    bool m_visible = false, m_refresh = false;
    bool m_gitline = false, m_gitdir = false;

    QRgb m_fg = 0;
    QColor m_blend;
    QFont m_font;
    QSize m_sizeHint{1, 1};

    QString makeGitString(const TermDirectory *dir, const QString &head);

    void reposition();
    void relayout(bool refresh);
    void recount();

private slots:
    void recolor(QRgb bg, QRgb fg);
    void refont(const QFont &font);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    FileBanner(FileModel *model, FileNameItem *item, FileWidget *parent);

    void setGitline(bool gitline);
    void setDirectory(const TermDirectory *dir);
    void setPath(const QString &part1, const QString &part2);

    QSize sizeHint() const;

public slots:
    void handleInfoChanged();
};
