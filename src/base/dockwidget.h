// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QDockWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
class TermManager;
class MainWindow;
class TermInstance;
class ServerInstance;

//
// Base class
//
class ToolWidget: public QWidget
{
    Q_OBJECT

protected:
    TermManager *m_manager;
    MainWindow *m_window;

    int m_returnPane = 0;

    bool eventFilter(QObject *object, QEvent *event);

signals:
    void userActivity();

public:
    ToolWidget(TermManager *manager);

    inline TermManager* manager() const { return m_manager; }
    inline MainWindow* window() const { return m_window; }

    virtual void visibilitySet();

    virtual TermInstance* selectedTerm() const;
    virtual ServerInstance* selectedServer() const;
    virtual regionid_t selectedRegion() const;

    virtual void toolAction(int index) = 0;
    virtual void contextMenu();

    virtual void selectFirst();
    virtual void selectPrevious();
    virtual void selectNext();
    virtual void selectLast();

    virtual void takeFocus(int returnPane);
    virtual void resetSearch();

    virtual void restoreState(int index);
    virtual void saveState(int index);
};

//
// Subclass with search bar
//
class SearchableWidget: public ToolWidget
{
    Q_OBJECT

protected:
    QLineEdit *m_line;
    QWidget *m_bar = nullptr, *m_header = nullptr;
    bool m_barShown = true, m_headerShown = true;

private:
    virtual void handleEscapePressed();
    virtual void handleFocusIn();
    virtual void handleFocusOut();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void returnFocus();

public:
    SearchableWidget(TermManager *manager);

    inline bool hasBar() const { return m_bar; }
    inline bool barShown() const { return m_barShown; }
    virtual bool hasHeader() const;
    inline bool headerShown() const { return m_headerShown; }

    void takeFocus(int returnPane);
    void resetSearch();

    bool toggleSearch();
    void toggleHeader();
};

//
// Dockable container class
//
class DockWidget final: public QDockWidget
{
    Q_OBJECT

private:
    QAction *a_action;
    MainWindow *m_parent;
    uint64_t m_toolFlag;

    bool m_istool = false;
    bool m_searchable = false;
    bool m_filterable = false;
    bool m_visible = false;
    bool m_autopopped = false;

private slots:
    void handleVisibility(bool visible);

protected:
    bool event(QEvent *event);

public slots:
    void bringUp();
    void bringToggle();

public:
    DockWidget(const QString &title, QAction *action,
               uint64_t toolFlag, MainWindow *parent);

    inline QAction* action() const { return a_action; }
    inline uint64_t toolFlag() const { return m_toolFlag; }

    inline bool istool() const { return m_istool; }
    inline bool searchable() const { return m_searchable; }
    inline bool filterable() const { return m_filterable; }
    inline bool visible() const { return m_visible; }
    inline bool autopopped() const { return m_autopopped; }

    inline void setAutopopped(bool autopopped) { m_autopopped = autopopped; }

    bool hasBar() const;
    bool barShown() const;
    bool hasHeader() const;
    bool headerShown() const;

    void setToolWidget(ToolWidget *widget, bool searchable, bool filterable);
};
