// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settings/rule.h"

#include <QWidget>
#include <QVector>
#include <QHash>
#include <QMultiMap>

QT_BEGIN_NAMESPACE
class QScrollArea;
class QStackedWidget;
class QBoxLayout;
class QLabel;
QT_END_NAMESPACE
class TermInstance;
class TermScrollport;
class TermKeymap;
class TermManager;
class ActionLabel;
class KeymapDirections;

//
// Contents Widget
//
class KeymapWidget final: public QWidget
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport;
    const TermKeymap *m_keymap;
    TermManager *m_manager;

    bool m_visible = false;
    bool m_changed = true;
    bool m_invoking = false;
    Tsq::TermFlags m_flags = 0;

    QScrollArea *m_scroll;
    QStackedWidget *m_stack;
    QLabel *m_header, *m_mode;
    QString m_currentStr, m_normalStr, m_commandStr, m_selectStr, m_headerStr;

    QMetaObject::Connection m_mocKeymap, m_mocFlags;
    QMetaObject::Connection m_mocRules, m_mocInvoke;

    QVector<ActionLabel*> m_labels;
    QHash<QString,ActionLabel*> m_map;
    ActionLabel *m_toggleCommand, *m_toggleSelect;
    KeymapDirections *m_dpad;

    int a_count[4];
    QVector<ActionLabel*> a_labels[4];
    QBoxLayout *a_layout[4];
    QHash<TermShortcut,ActionLabel*> m_shortcutMap;
    QMultiMap<QString,ActionLabel*> m_additionalMap;

    void processRule(const KeymapRule *rule);
    void processKeymap(const TermKeymap *keymap);
    void setInvoking(bool invoking);

private slots:
    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);
    void handleLinkActivated(const QString &link);
    void handleInvocation(const QString &slot);

    void handleRulesChanged();
    void handleKeymapChanged(const TermKeymap *keymap);
    void handleFlagsChanged(Tsq::TermFlags flags);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    KeymapWidget(TermManager *manager);
    void doPolish();
};
