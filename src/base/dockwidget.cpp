// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "dockwidget.h"
#include "manager.h"
#include "mainwindow.h"
#include "menubase.h"
#include "term.h"

#include <QLineEdit>
#include <QAction>
#include <QKeyEvent>

#define TR_TEXT1 TL("window-text", "Search")

//
// Base class
//
ToolWidget::ToolWidget(TermManager *manager) :
    m_manager(manager),
    m_window(manager->parent())
{
}

TermInstance *
ToolWidget::selectedTerm() const
{
    return nullptr;
}

ServerInstance *
ToolWidget::selectedServer() const
{
    auto *term = selectedTerm();
    return term ? term->server() : nullptr;
}

regionid_t
ToolWidget::selectedRegion() const
{
    return INVALID_REGION_ID;
}

bool
ToolWidget::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::KeyPress:
        emit userActivity();
        // fallthru
    default:
        return false;
    }
}

void
ToolWidget::contextMenu()
{}

void
ToolWidget::selectFirst()
{}

void
ToolWidget::selectPrevious()
{}

void
ToolWidget::selectNext()
{}

void
ToolWidget::selectLast()
{}

void
ToolWidget::takeFocus(int)
{}

void
ToolWidget::resetSearch()
{}

void
ToolWidget::restoreState(int)
{}

void
ToolWidget::saveState(int)
{}

void
ToolWidget::visibilitySet()
{}

//
// Subclass with search bar
//
SearchableWidget::SearchableWidget(TermManager *manager) :
    ToolWidget(manager)
{
    m_line = new QLineEdit;
    m_line->setPlaceholderText(TR_TEXT1);
    // m_line->setClearButtonEnabled(true);
    m_line->installEventFilter(this);

    connect(m_line, SIGNAL(returnPressed()), SLOT(returnFocus()));
}

void
SearchableWidget::returnFocus()
{
    // Select the first search result
    if (!m_line->text().isEmpty())
        selectFirst();

    // Return to the terminal pane
    m_manager->actionSwitchPane(QString::number(m_returnPane));
}

void
SearchableWidget::takeFocus(int returnPane)
{
    m_returnPane = returnPane;

    if (m_bar)
        m_bar->setVisible(m_barShown = true);

    m_line->setFocus(Qt::OtherFocusReason);
}

void
SearchableWidget::resetSearch()
{
    m_line->clear();
}

bool
SearchableWidget::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (object == m_line) {
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
                m_line->clear();
                returnFocus();
                handleEscapePressed();
                return true;
            }
        }
        // fallthru
    case QEvent::MouseButtonPress:
        emit userActivity();
        break;
    case QEvent::FocusIn:
        if (object == m_line)
            handleFocusIn();
        break;
    case QEvent::FocusOut:
        if (object == m_line)
            handleFocusOut();
        break;
    default:
        break;
    }

    return false;
}

bool
SearchableWidget::toggleSearch()
{
    if (m_bar) {
        m_line->clear();
        m_bar->setVisible(m_barShown = !m_barShown);
    }

    return m_barShown;
}

void
SearchableWidget::toggleHeader()
{
    if (m_header)
        m_header->setVisible(m_headerShown = !m_headerShown);
}

bool
SearchableWidget::hasHeader() const
{
    return m_header;
}

void
SearchableWidget::handleEscapePressed()
{
    if (m_bar)
        m_bar->setVisible(m_barShown = false);
}

void
SearchableWidget::handleFocusIn()
{}

void
SearchableWidget::handleFocusOut()
{}

//
// Dockable container class
//
DockWidget::DockWidget(const QString &title, QAction *action,
                       uint64_t toolFlag, MainWindow *parent) :
    QDockWidget(title, parent),
    a_action(action),
    m_parent(parent),
    m_toolFlag(toolFlag & DynToolMask)
{
    action->setCheckable(true);

    connect(this, SIGNAL(visibilityChanged(bool)), SLOT(handleVisibility(bool)));
}

void
DockWidget::setToolWidget(ToolWidget *widget, bool searchable, bool filterable)
{
    QDockWidget::setWidget(widget);
    m_istool = true;
    m_searchable = searchable;
    m_filterable = filterable;
    connect(widget, SIGNAL(userActivity()), SLOT(bringUp()));
}

void
DockWidget::bringUp()
{
    if (!m_visible) {
        show();
        raise();
    }

    m_parent->liftDockWidget(this);
    m_autopopped = false;
}

void
DockWidget::handleVisibility(bool visible)
{
    if ((m_visible = visible))
    {
        m_parent->liftDockWidget(this);

        if (m_istool)
            static_cast<ToolWidget*>(widget())->visibilitySet();
    } else {
        m_autopopped = false;
    }
}

void
DockWidget::bringToggle()
{
    bool checked = a_action->isChecked();

    if (checked == isHidden()) {
        if (checked)
            bringUp();
        else
            close();
    }
}

bool
DockWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Hide:
        a_action->setChecked(false);
        break;
    case QEvent::Show:
        a_action->setChecked(true);
        break;
    default:
        break;
    }

    return QDockWidget::event(event);
}

bool
DockWidget::hasBar() const
{
    return m_searchable && static_cast<SearchableWidget*>(widget())->hasBar();
}

bool
DockWidget::barShown() const
{
    return m_searchable && static_cast<SearchableWidget*>(widget())->barShown();
}

bool
DockWidget::hasHeader() const
{
    return m_searchable && static_cast<SearchableWidget*>(widget())->hasHeader();
}

bool
DockWidget::headerShown() const
{
    return m_searchable && static_cast<SearchableWidget*>(widget())->headerShown();
}
