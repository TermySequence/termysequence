// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/color.h"
#include "app/config.h"
#include "app/datastore.h"
#include "suggestwidget.h"
#include "manager.h"
#include "mainwindow.h"
#include "term.h"
#include "mark.h"
#include "settings/global.h"
#include "lib/grapheme.h"

#include <QPainter>
#include <QStyleOption>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>

SuggestWidget::SuggestWidget(TermManager *manager) :
    ToolWidget(manager),
    m_searchId(INVALID_SEARCH_ID)
{
    setMouseTracking(true);

    // Calculate size limits
    QSize size = QApplication::desktop()->availableGeometry(this).size() / THUMB_MIN_SCALE;

    if (size.width() < THUMB_MIN_SIZE)
        size.setWidth(THUMB_MIN_SIZE);
    if (size.height() < THUMB_MIN_SIZE)
        size.setHeight(THUMB_MIN_SIZE);

    setMinimumSize(size);
    m_sizeHint = size * 1.2;

    connect(manager,
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*)));
    connect(g_datastore,
            SIGNAL(reportResult(unsigned,AttributeMap)),
            SLOT(handleSearchResult(unsigned,AttributeMap)));
    connect(g_datastore,
            SIGNAL(reportFinished(unsigned,bool)),
            SLOT(handleSearchFinished(unsigned,bool)));
}

void
SuggestWidget::relayout()
{
    m_cols = width() / m_cellSize.width();
    m_rows = height() / m_cellSize.height();
    m_size = m_cols * m_rows;

    if (m_searchId != INVALID_SEARCH_ID) {
        g_datastore->startSearch(m_searchStr, m_size, m_searchId);
        clear();
        update();
    }
}

void
SuggestWidget::refont(const QFont &font)
{
    setDisplayFont(font);
    relayout();
}

void
SuggestWidget::recolor()
{
    m_matchBg = m_term->palette().specialBg(Tsqt::SearchText, m_term->bg());
    m_matchFg = m_term->palette().specialFg(Tsqt::SearchText, m_term->fg());

    for (DisplayCell &cell: m_displayCells) {
        if (cell.flags & (Tsq::Bg|Tsq::Fg)) {
            cell.bg = m_matchBg;
            cell.fg = m_matchFg;
        }
    }

    setStyleSheet(L("SuggestWidget {background-color:#%1}")
                  .arg(m_term->bg(), 6, 16, Ch0));
}

inline void
SuggestWidget::clear()
{
    m_info.clear();
    m_displayCells.clear();
    m_commands.clear();
    m_col = m_row = m_pos = 0;
    m_haveRoom = true;
    m_hoverIndex = m_activeIndex = -1;
}

inline void
SuggestWidget::stopRaiseTimer()
{
    if (m_timerId != 0) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

inline void
SuggestWidget::setRaiseTimer(bool delay)
{
    if (g_global->raiseSuggestions() && !m_window->commandsDock()->visible())
    {
        stopRaiseTimer();
        if (delay) {
            m_timerId = startTimer(g_global->suggTime());
        } else {
            m_window->commandsDock()->raise();
        }
    }
}

void
SuggestWidget::stop()
{
    g_datastore->stopSearch(m_searchId);
    update();

    if (g_global->raiseSuggestions()) {
        stopRaiseTimer();
        m_window->unpopDockWidget(m_window->commandsDock(), m_first);
    }
}

void
SuggestWidget::setSearchString(const QString &searchStr)
{
    m_searchStr = searchStr;
    std::string str = searchStr.toStdString();
    size_t idx = str.find_first_not_of(' ');
    if (idx != std::string::npos) {
        if (idx)
            str.erase(0, idx);

        Tsq::GraphemeWalk tbf(m_term->unicoding(), str);

        for (int i = 0; i < DATASTORE_MINIMUM_LENGTH; ++i)
            if (!tbf.next())
                goto stop;

        stopRaiseTimer();
        g_datastore->startSearch(m_searchStr, m_size, m_searchId);
        clear();
        update();
        return;
    }
stop:
    stop();
}

inline void
SuggestWidget::clearSearchString()
{
    m_searchStr.clear();
    stop();
}

void
SuggestWidget::handleTermActivated(TermInstance *term)
{
    if (m_term) {
        m_term->disconnect(this);
    }

    if ((m_term = term)) {
        connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
        connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor()));
        connect(m_term, SIGNAL(paletteChanged()), SLOT(recolor()));
        connect(m_term, SIGNAL(attributeChanged(QString,QString)), SLOT(handleAttributeChanged(const QString&,const QString&)));
        connect(m_term, SIGNAL(attributeRemoved(QString)), SLOT(handleAttributeRemoved(const QString&)));

        refont(m_term->font());
        recolor();

        auto i = m_term->attributes().constFind(g_attr_COMMAND);
        if (i != m_term->attributes().cend())
            setSearchString(*i);
        else
            clearSearchString();

        m_first = false;
    }
}

void
SuggestWidget::handleAttributeRemoved(const QString &key)
{
    if (key == g_attr_COMMAND)
        clearSearchString();
}

void
SuggestWidget::handleAttributeChanged(const QString &key, const QString &value)
{
    if (key == g_attr_COMMAND)
        setSearchString(value);
}

void
SuggestWidget::addString(DisplayCell &dc)
{
    int rem = m_cols - m_col, state = 0;

    dc.substr = dc.text.toStdString();

    while (!dc.substr.empty() && m_row < m_rows) {
        dc.rect.moveTo(m_col * m_cellSize.width(), m_row * m_cellSize.height());
        dc.point = dc.rect.topLeft();
        dc.point.ry() += m_ascent;

        int used = decomposeStringCells(dc, rem, &state);

        if (used < rem) {
            m_col += used;
        } else {
            ++m_row;
            m_col = 0;
            rem = m_cols;
        }
    }
}

static inline QString
getIndexString(int index, bool alias)
{
    QChar num = TermMark::base36(index, ' ');
    QChar sep = alias ? '*' : ':';

    return QString(num) + sep;
}

DisplayCellList
SuggestWidget::getCommandString(const DisplayCell &params, const QString &commandStr,
                                const QString &acronymStr) const
{
    DisplayCellList result;
    DisplayCell cell = params;
    int idx;

    if ((idx = commandStr.indexOf(m_searchStr)) != -1) {
        cell.text = commandStr.left(idx);
        result.emplace_back(cell);

        cell.flags |= Tsq::Bg|Tsq::Fg;
        cell.bg = m_matchBg;
        cell.fg = m_matchFg;
        cell.text = commandStr.mid(idx, m_searchStr.size());
        result.emplace_back(cell);

        cell = params;
        cell.text = commandStr.mid(idx + m_searchStr.size());
        result.emplace_back(cell);
    }
    else if ((idx = acronymStr.indexOf(m_searchStr)) != -1) {
        auto poslist = DatastoreController::getAcronymPositions(commandStr);
        int cur = 0, prev;

        while (!poslist.isEmpty() && idx--)
            poslist.pop_front();

        idx = m_searchStr.size();
        while (!poslist.isEmpty() && idx-- && cur < commandStr.size()) {
            prev = cur;
            cur = poslist.front();
            poslist.pop_front();

            cell.text = commandStr.mid(prev, cur - prev);
            result.emplace_back(cell);

            cell.flags |= Tsq::Bg|Tsq::Fg;
            cell.bg = m_matchBg;
            cell.fg = m_matchFg;
            cell.text = commandStr.at(cur);
            result.emplace_back(cell);
            cell = params;

            ++cur;
        }
        if (cur < commandStr.size()) {
            cell.text = commandStr.mid(cur);
            result.emplace_back(cell);
        }
    }
    else {
        cell.text = commandStr;
        result.emplace_back(cell);
    }

    return result;
}

void
SuggestWidget::handleSearchResult(unsigned id, AttributeMap map)
{
    if (m_searchId != id || !m_haveRoom)
        return;

    // (Re)start raise timer if this is the first result
    if (m_pos == 0)
        setRaiseTimer(true);

    const QString &commandStr = map[g_ds_COMMAND];
    bool alias = map.value(g_ds_TYPE) == g_ds_ALIAS;
    if (m_commands.contains(commandStr))
        return;

    // Compute required space
    int index = m_info.size();
    QString indexStr = getIndexString(index + 1, alias);

    int req = indexStr.size() + stringCellWidth(commandStr);
    if (req > m_size - m_pos) {
        m_haveRoom = false;
        return;
    }

    // Index number
    DisplayCell params;
    params.flags = Tsq::Inverse|Tsq::Bold;
    params.bg = m_term->bg();
    params.fg = m_term->fg();
    params.text = indexStr;
    params.rect.setHeight(m_cellSize.height());
    params.data = index;
    addString(params);

    // Command string
    params.flags = 0;
    for (DisplayCell &cell: getCommandString(params, commandStr, map[g_ds_ACRONYM]))
        if (!cell.text.isEmpty())
            addString(cell);

    SuggestInfo info;
    info.start = m_pos;
    info.end = (m_pos += req);
    info.map = std::move(map);
    m_info.append(std::move(info));

    // Extra space
    if (m_col > 0) {
        ++m_col;
        ++m_pos;
    }

    m_commands.insert(commandStr);
    update();
}

void
SuggestWidget::handleSearchFinished(unsigned id, bool overlimit)
{
    if (m_searchId == id)
        setRaiseTimer(m_commands.size() > 4 || overlimit);
}

void
SuggestWidget::timerEvent(QTimerEvent *)
{
    killTimer(m_timerId);
    m_timerId = 0;
    m_window->commandsDock()->raise();
}

void
SuggestWidget::resizeEvent(QResizeEvent *event)
{
    int cols = event->size().width() / m_cellSize.width();
    int rows = event->size().height() / m_cellSize.height();

    if (m_cols != cols || m_rows != rows)
        relayout();
}

static void
setHover(DisplayCellList &cells, int owner, bool hover)
{
    for (DisplayCell &dc: cells) {
        if (dc.data < owner)
            continue;
        if (dc.data > owner)
            break;

        if (hover)
            dc.flags |= Tsq::Underline;
        else
            dc.flags &= ~(Tsqt::CellFlags)Tsq::Underline;
    }
}

static void
setActive(DisplayCellList &cells, int owner, bool active)
{
    for (DisplayCell &dc: cells) {
        if (dc.data < owner)
            continue;
        if (dc.data > owner)
            break;

        if (active)
            dc.flags |= Tsqt::Selected;
        else
            dc.flags &= ~(Tsqt::CellFlags)Tsqt::Selected;
    }
}

void
SuggestWidget::setActiveIndex(int index)
{
    if (m_activeIndex != index) {
        if (m_activeIndex != -1)
            setActive(m_displayCells, m_activeIndex, false);
        if ((m_activeIndex = index) != -1)
            setActive(m_displayCells, m_activeIndex, true);

        update();
    }
}

const QString &
SuggestWidget::getCommand(int index)
{
    if (index < 0) {
        index = m_activeIndex;
        if (index == -1)
            index = 0;
    }
    if (index >= m_info.size())
        return g_mtstr;

    const QString &command = m_info[index].map[g_ds_COMMAND];

    // Store alias
    g_datastore->storeAlias(m_searchStr, command);

    setActiveIndex(index);
    return command;
}

void
SuggestWidget::removeCommand(int index)
{
    if (index < 0) {
        index = m_activeIndex;
        if (index == -1)
            index = 0;
    }
    if (index >= m_info.size())
        return;

    // Remove command
    g_datastore->removeCommand(m_info[index].map[g_ds_COMMAND]);

    if (m_searchId != INVALID_SEARCH_ID) {
        g_datastore->startSearch(m_searchStr, m_size, m_searchId);
        clear();
        update();
    }
}

void
SuggestWidget::selectFirst()
{
    if (!m_info.isEmpty())
        setActiveIndex(0);
}

void
SuggestWidget::selectPrevious()
{
    int n = m_info.size();

    if (n == 0)
        return;
    else if (m_activeIndex == -1)
        setActiveIndex(n - 1);
    else
        setActiveIndex((m_activeIndex + n - 1) % n);
}

void
SuggestWidget::selectNext()
{
    int n = m_info.size();

    if (n == 0)
        return;
    else if (m_activeIndex == -1)
        setActiveIndex(0);
    else
        setActiveIndex((m_activeIndex + 1) % n);
}

void
SuggestWidget::selectLast()
{
    if (!m_info.isEmpty())
        setActiveIndex(m_info.size() - 1);
}

int
SuggestWidget::getClickIndex(const QPoint &p) const
{
    int row = p.y() / m_cellSize.height();
    int col = p.x() / m_cellSize.width();

    if (row >= 0 && row < m_rows && col >= 0 && col < m_cols)
    {
        int pos = row * m_cols + col;

        for (int i = 0; i < m_info.size(); ++i)
            if (pos >= m_info[i].start && pos < m_info[i].end)
                return i;
    }
    return -1;
}

void
SuggestWidget::action(int type, int index)
{
    QString tmp;

    switch (type) {
    case SuggActWrite:
        tmp = A("WriteSuggestion|%1");
        break;
    case SuggActWriteNewline:
        tmp = A("WriteSuggestionNewline|%1");
        break;
    case SuggActCopy:
        tmp = A("CopySuggestion|%1");
        break;
    case SuggActRemove:
        tmp = A("RemoveSuggestion|%1");
        break;
    default:
        return;
    }

    m_manager->invokeSlot(tmp.arg(index));
}

void
SuggestWidget::mouseAction(int action)
{
    this->action(action, getClickIndex(m_dragStartPosition));
}

void
SuggestWidget::mousePressEvent(QMouseEvent *event)
{
    setActiveIndex(getClickIndex(event->pos()));

    m_dragStartPosition = event->pos();
    m_clicking = true;
    emit userActivity();
    event->accept();
}

void
SuggestWidget::mouseMoveEvent(QMouseEvent *event)
{
    int index = getClickIndex(event->pos());

    if (m_hoverIndex != index) {
        if (m_hoverIndex != -1)
            setHover(m_displayCells, m_hoverIndex, false);
        if ((m_hoverIndex = index) != -1)
            setHover(m_displayCells, m_hoverIndex, true);

        update();
    }
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        m_clicking = dist <= QApplication::startDragDistance();
    }
}

void
SuggestWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->suggAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->suggAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->suggAction3());
            break;
        default:
            break;
        }
    }

    event->accept();
}

void
SuggestWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->suggAction0());

    event->accept();
}

void
SuggestWidget::contextMenuEvent(QContextMenuEvent *event)
{
    int index = getClickIndex(event->pos());
    setActiveIndex(index);

    QMenu *m;

    auto i = m_popups.constFind(index);
    if (i != m_popups.cend())
        m = *i;
    else
        m_popups.insert(index, m = m_window->getCommandPopup(index));

    m->popup(event->globalPos());
    event->accept();
}

void
SuggestWidget::contextMenu()
{
    int index = m_activeIndex;
    QMenu *m;

    auto i = m_popups.constFind(index);
    if (i != m_popups.cend())
        m = *i;
    else
        m_popups.insert(index, m = m_window->getCommandPopup(index));

    QPoint mid(width() / 2, height() / 4);
    m->popup(mapToGlobal(mid));
}

void
SuggestWidget::leaveEvent(QEvent *event)
{
    if (m_hoverIndex != -1) {
        setHover(m_displayCells, m_hoverIndex, false);
        m_hoverIndex = -1;
        update();
    }
}

void
SuggestWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (m_term && m_searchId != INVALID_SEARCH_ID)
    {
        painter.setFont(m_font);

        CellState state(0);
        state.fg = m_term->fg();
        state.bg = m_term->bg();

        for (DisplayCell dc: m_displayCells)
        {
            // Flags adjustments for selected
            if (dc.flags & Tsqt::Selected) {
                dc.flags |= Tsq::Bold;

                if (!(dc.flags & (Tsq::Fg|Tsq::Bg|Tsqt::UnderlineDummy))) {
                    dc.flags |= Tsq::Bg;
                    dc.bg = Colors::blend1(state.bg, state.fg);
                }
            }

            paintCell(painter, dc, state);
        }
    }
}

QSize
SuggestWidget::sizeHint() const
{
    return m_sizeHint;
}

QString
SuggestWidget::getKillSequence() const
{
    // try to get KILL character from termios information
    char kill = m_term->process()->termiosChars[Tsq::TermiosVKILL];

    if (m_searchStr.isEmpty())
        return g_mtstr;
    else if (kill)
        return C(kill);
    else
        return QString('\b').repeated(stringCellWidth(m_searchStr));
}

void
SuggestWidget::toolAction(int index)
{
    int action;

    switch (index) {
    case 0:
        action = g_global->suggAction0();
        break;
    case 1:
        action = g_global->suggAction1();
        break;
    case 2:
        action = g_global->suggAction2();
        break;
    case 3:
        action = g_global->suggAction3();
        break;
    default:
        return;
    }

    this->action(action, m_activeIndex);
}
