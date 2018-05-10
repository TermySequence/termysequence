// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/color.h"
#include "app/config.h"
#include "filebanner.h"
#include "filewidget.h"
#include "filemodel.h"
#include "filetracker.h"
#include "fileviewitem.h"
#include "settings/global.h"

#include <QPainter>
#include <QStyleOption>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QtMath>

#define TR_TEXT1 TL("window-text", "%1 folders (%2 hidden), %3 files (%4 hidden)")
#define TR_TEXT2 TL("window-text", "%1 folders, %2 files")
#define TR_TEXT3 TL("window-text", "Detached") + A(": ")
#define TR_TEXT4 TL("window-text", "Branch") + A(": ")
#define TR_TEXT5 TL("window-text", "synchronized")
#define TR_TEXT6 TL("window-text", "ahead %1, behind %2")
#define TR_TEXT7 TL("window-text", "ahead %1")
#define TR_TEXT8 TL("window-text", "behind %1")
#define TR_TEXT9 '(' + TL("window-text", "not in cwd") + ')'

//
// Path display
//
FileBannerPath::FileBannerPath(FileBanner *parent) :
    QWidget(parent),
    m_model(parent->m_model),
    m_nameitem(parent->m_nameitem),
    m_parent(parent->m_parent)
{
    setMouseTracking(true);
    installEventFilter(m_parent);

    connect(m_model, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_model, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));
}

void
FileBannerPath::relayout(bool refresh)
{
    if (!m_visible) {
        m_refresh |= refresh;
        return;
    }

    m_names.clear();
    m_namerects.clear();
    m_clicked = m_mouseover = -1;
    m_displayCells.clear();

    if (refresh && m_term) {
        DisplayCell dc;
        dc.flags = Tsq::Bold;
        dc.text = m_part1;
        dc.rect.setHeight(m_cellSize.height());
        dc.point.setY(m_ascent);

        int pathw = qCeil(decomposeStringPixels(dc));

        dc.flags = Tsq::Bold|Tsq::Underline;
        dc.text = m_part2;
        dc.rect.setX(pathw);
        dc.point.setX(pathw);

        int selw = qCeil(decomposeStringPixels(dc));
        int w = width();
        m_selrect.setRect(w - selw, 0, selw, height());

        pathw += selw;
        if (pathw > w) {
            elideCellsLeft(pathw, w);
        }

        // Walk again to get rectangles
        QStringList parts = m_path.split('/');
        bool trailer;
        if ((trailer = parts.back().isEmpty()))
            parts.pop_back();

        QString name;
        qreal namew, x = w - pathw;
        QRect rect(x, 0, 1, height());

        for (int i = 0, n = parts.size(); i < n; ++i) {
            if (trailer || i < n - 1)
                parts[i].append('/');

            namew = stringCellWidth(parts[i]) * m_cellSize.width();
            rect.setWidth(qCeil(namew));
            m_namerects.append(rect);
            m_names.append(name += parts[i]);
            rect.setX(x += namew);
        }

        if (!m_namerects.isEmpty())
            m_namerects.back().setRight(w - 1);
    }

    update();
}

void
FileBannerPath::setPath(const QString &part1, const QString &part2)
{
    m_part1 = part1;
    m_part2 = part2;
    m_path = part1 + part2;

    m_sizeHint.setWidth(qCeil(stringCellWidth(part1) * m_cellSize.width()) +
                        qCeil(stringCellWidth(part2) * m_cellSize.width()));
    relayout(true);
}

void
FileBannerPath::recolor(QRgb bg, QRgb fg)
{
    m_fg = fg;
    m_blend1 = Colors::blend1(bg, fg);
    m_blend2 = Colors::blend2(bg, fg);

    update();
}

void
FileBannerPath::refont(const QFont &font)
{
    if (m_font != font) {
        setDisplayFont(font);
        m_margin = 1 + m_cellSize.height() / MARGIN_INCREMENT;

        m_sizeHint.setWidth(qCeil(stringCellWidth(m_part1) * m_cellSize.width()) +
                            qCeil(stringCellWidth(m_part2) * m_cellSize.width()));
        update();
    }
}

void
FileBannerPath::resizeEvent(QResizeEvent *event)
{
    relayout(true);
}

int
FileBannerPath::getClickIndex(QPoint pos) const
{
    for (int i = 0, n = m_namerects.size(); i < n; ++i)
        if (m_namerects[i].contains(pos))
            return i;

    return -1;
}

void
FileBannerPath::mouseAction(int action) const
{
    if (m_clicked != -1) {
        if (m_clicked == m_names.size() - 1 && !m_part2.isEmpty())
            m_parent->action(action);
        else
            m_parent->action(action, m_names[m_clicked]);
    }
}

void
FileBannerPath::mousePressEvent(QMouseEvent *event)
{
    m_clicked = getClickIndex(event->pos());
    update();

    m_dragStartPosition = event->pos();
    m_clicking = true;

    m_nameitem->setBlinkEffect();
    event->accept();
}

void
FileBannerPath::mouseMoveEvent(QMouseEvent *event)
{
    int dist = (event->pos() - m_dragStartPosition).manhattanLength();
    bool isdrag = dist >= QApplication::startDragDistance();

    if (event->buttons()) {
        if (isdrag && (event->buttons() & Qt::LeftButton) && m_clicked != -1)
            m_parent->startDrag(m_names[m_clicked]);
    }
    else {
        int mouseover = getClickIndex(event->pos());
        if (m_mouseover != mouseover) {
            m_mouseover = mouseover;
            update();
        }
    }

    if (m_clicking && isdrag) {
        m_clicking = false;
        m_clicked = -1;
        update();
    }

    event->accept();
}

void
FileBannerPath::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->fileAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->fileAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->fileAction3());
            break;
        default:
            break;
        }

        m_clicking = false;
        m_clicked = -1;
        update();
    }

    event->accept();
}

void
FileBannerPath::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0) {
        m_clicked = getClickIndex(event->pos());
        mouseAction(g_global->fileAction0());
        m_clicked = -1;
    }

    event->accept();
}

void
FileBannerPath::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *m;
    QString path;

    if (m_clicked != -1) {
        if (m_clicked == m_names.size() - 1) {
            if (!m_part2.isEmpty() && m_part2 != "/") {
                m = m_parent->getFilePopup(m_parent->selectedFile());
                goto out;
            }
        }
        path = m_names[m_clicked];
    }

    m = m_parent->getDirPopup(path);
out:
    m->popup(event->globalPos());
    event->accept();

    m_mouseover = -1;
    m_clicked = -1;
    update();
}

void
FileBannerPath::leaveEvent(QEvent *event)
{
    m_mouseover = -1;
    update();
}

void
FileBannerPath::showEvent(QShowEvent *event)
{
    m_visible = true;
    relayout(m_refresh);
}

void
FileBannerPath::hideEvent(QHideEvent *event)
{
    m_visible = false;
    m_refresh = false;
}

void
FileBannerPath::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_selrect.width())
        painter.fillRect(m_selrect, m_blend1);
    if (m_clicked != -1)
        painter.fillRect(m_namerects[m_clicked], m_blend2);

    CellState state(0);
    state.fg = m_fg;
    painter.setFont(m_font);

    for (const DisplayCell &cell: m_displayCells)
        paintCell(painter, cell, state);

    if (m_mouseover != -1) {
        QPen pen(painter.pen());
        pen.setColor(m_fg);
        pen.setWidth(2 * m_margin);
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setClipRect(m_namerects[m_mouseover]);
        painter.drawRect(m_namerects[m_mouseover]);
    }
}

QSize
FileBannerPath::sizeHint() const
{
    return m_sizeHint;
}

//
// Arrangement
//
FileBanner::FileBanner(FileModel *model, FileNameItem *nameitem, FileWidget *parent) :
    QWidget(parent),
    m_model(model),
    m_nameitem(nameitem),
    m_parent(parent),
    m_notincwd(TR_TEXT9),
    m_info(&m_counts),
    m_infod(m_info)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // this connects first
    m_path = new FileBannerPath(this);

    connect(model, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(model, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));

    connect(model, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            SLOT(handleInfoChanged()));
    connect(model, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            SLOT(handleInfoChanged()));

    installEventFilter(parent);
}

void
FileBanner::reposition()
{
    int y = m_margin;
    int height = m_cellSize.height();

    if (m_gitline && m_gitdir) {
        m_gitrect.setY(y);
        m_gitrect.setHeight(height);

        y += height;
    }

    m_path->move(0, y);
    m_inforect.setY(y);
    m_inforect.setHeight(height);

    m_sizeHint.setHeight(height + y);
    updateGeometry();
}

void
FileBanner::relayout(bool refresh)
{
    if (!m_visible) {
        m_refresh |= refresh;
        return;
    }

    int w = width();

    if (refresh && m_term) {
        if (m_gitline && m_gitdir) {
            m_displayCells.clear();
            DisplayCell dc;
            dc.flags = 0;
            dc.text = m_git;
            dc.rect = m_gitrect;
            dc.point = m_gitrect.topLeft();
            dc.point.ry() += m_ascent;

            int gitw = qCeil(decomposeStringPixels(dc));
            if (gitw > w) {
                elideCellsLeft(gitw, w);
                gitw = w;
            }
            m_gitrect.setWidth(gitw);
        }

        int pathw = qMin(m_path->sizeHint().width(), w);
        m_path->resize(pathw, m_cellSize.height());

        m_inforect.setX(pathw + 2 * m_cellSize.width());
    }

    m_inforect.setWidth(w -= m_inforect.x());
    if (m_metrics.width(*m_info) > w)
        m_infod = &g_mtstr;
    else
        m_infod = m_info;

    update();
}

void
FileBanner::recount()
{
    const unsigned *counts = m_model->files()->counts();

    if (counts[TermFile::FnHiddenDirs] || counts[TermFile::FnHiddenFiles]) {
        m_counts = TR_TEXT1
            .arg(counts[TermFile::FnDirs] + counts[TermFile::FnHiddenDirs])
            .arg(counts[TermFile::FnHiddenDirs])
            .arg(counts[TermFile::FnFiles] + counts[TermFile::FnHiddenFiles])
            .arg(counts[TermFile::FnHiddenFiles]);
    } else {
        m_counts = TR_TEXT2
            .arg(counts[TermFile::FnDirs])
            .arg(counts[TermFile::FnFiles]);
    }
}

void
FileBanner::handleInfoChanged()
{
    if (m_visible) {
        recount();
        relayout(false);
    }
}

inline QString
FileBanner::makeGitString(const TermDirectory *dir, const QString &head)
{
    bool detached = dir->attributes.contains(g_attr_DIR_GIT_DETACHED);
    QString result = detached ? TR_TEXT3 : TR_TEXT4;

    result.append(head);

    auto i = dir->attributes.constFind(g_attr_DIR_GIT_TRACK);
    if (i != dir->attributes.cend()) {
        result.append(L(" \u2192 "));
        result.append(*i);
        result.append(A(", "));

        unsigned ahead = dir->attributes.value(g_attr_DIR_GIT_AHEAD).toUInt();
        unsigned behind = dir->attributes.value(g_attr_DIR_GIT_BEHIND).toUInt();

        if (ahead == 0 && behind == 0)
            result.append(TR_TEXT5);
        else if (ahead && behind)
            result.append(TR_TEXT6.arg(ahead).arg(behind));
        else if (ahead)
            result.append(TR_TEXT7.arg(ahead));
        else
            result.append(TR_TEXT8.arg(behind));
    }

    return result;
}

void
FileBanner::setDirectory(const TermDirectory *dir)
{
    bool gitdir = false;
    QString git;
    m_term = m_model->term();
    m_path->setTerm(m_term);

    if (dir && !dir->iserror) {
        git = dir->attributes.value(g_attr_DIR_GIT);
        gitdir = !git.isEmpty();
        recount();
        // call to setPath follows
    }
    if (m_gitdir != gitdir) {
        m_gitdir = gitdir;
        reposition();
    }
    if (m_gitdir) {
        m_git = makeGitString(dir, git);
    }
}

void
FileBanner::setPath(const QString &part1, const QString &part2)
{
    m_path->setPath(part1, part2);

    if (part2.isEmpty())
        m_info = &m_counts;
    else if (part2.lastIndexOf('/', -2) != -1 || part2 == "/")
        m_info = &m_notincwd;
    else
        m_info = &g_mtstr;

    relayout(true);
}

void
FileBanner::recolor(QRgb bg, QRgb fg)
{
    m_fg = fg;
    m_blend = Colors::blend2(bg, fg);

    update();
}

void
FileBanner::refont(const QFont &font)
{
    if (m_font != font) {
        setDisplayFont(m_font = font);
        m_margin = 1 + m_cellSize.height() / MARGIN_INCREMENT;

        reposition();
        relayout(true);
    }
}

void
FileBanner::setGitline(bool gitline)
{
    if (m_gitline != gitline) {
        m_gitline = gitline;
        reposition();
    }
}

void
FileBanner::resizeEvent(QResizeEvent *event)
{
    relayout(true);
}

void
FileBanner::mousePressEvent(QMouseEvent *event)
{
    m_nameitem->setBlinkEffect();
    event->accept();
}

void
FileBanner::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *m = m_parent->getFilePopup(m_parent->selectedFile());
    m->popup(event->globalPos());
    event->accept();
}

void
FileBanner::showEvent(QShowEvent *event)
{
    m_visible = true;
    relayout(m_refresh);
}

void
FileBanner::hideEvent(QHideEvent *event)
{
    m_visible = false;
    m_refresh = false;
}

void
FileBanner::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    CellState state(0);
    state.fg = m_fg;

    painter.setFont(m_font);
    painter.setPen(QColor(m_fg));
    painter.fillRect(0, 0, width(), m_margin, m_blend);

    if (m_gitline && m_gitdir) {
        painter.setClipRect(m_gitrect);
        for (const DisplayCell &cell: m_displayCells)
            paintCell(painter, cell, state);
    }

    painter.setClipRect(m_inforect);
    painter.drawText(m_inforect, Qt::AlignRight, *m_infod);
}

QSize
FileBanner::sizeHint() const
{
    return m_sizeHint;
}
