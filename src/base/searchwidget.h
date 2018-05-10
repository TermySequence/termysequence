// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "dockwidget.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QLabel;
QT_END_NAMESPACE
class TermScrollport;

class SearchWidget final: public SearchableWidget
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport;

    QComboBox *m_combo;
    QCheckBox *m_check;
    QLabel *m_status;

    QMetaObject::Connection m_mocCombo;
    QMetaObject::Connection m_mocLine;
    QMetaObject::Connection m_mocCheck;
    QMetaObject::Connection m_mocStatus;
    QMetaObject::Connection m_mocTerm;
    bool m_maskTerm = false;

    void connectWidgets();
    void disconnectWidgets();
    void handleEscapePressed();
    void handleFocusIn();
    void handleFocusOut();

private slots:
    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);

    void handleSearchChanged();
    void handleSearchText();
    void handleReturnPressed();
    void handleSearchReset();

public:
    SearchWidget(TermManager *manager);

    void toolAction(int index);

    void restoreState(int index);
    void saveState(int index);
};
