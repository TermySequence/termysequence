// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "splitwidget.h"
#include "lib/types.h"
#include "lib/uuid.h"

#include <QHash>
#include <QVector>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE
class TermManager;
class TermStack;
class TermInstance;
class TermBlank;
class TermScrollport;
class TermPeek;
class ProfileSettings;

class StackWidget final: public SplitWidget
{
    Q_OBJECT

private:
    QWidget *m_widget;
    QStackedWidget *m_widgetStack;
    QHash<TermInstance*,QWidget*> m_childMap;
    TermBlank *m_blank;
    TermPeek *m_peek;

    Tsq::Uuid m_stateId;
    QHash<Tsq::Uuid,ScrollportState> m_stateRecords;

    QVector<ScrollportState> stateRecords() const;

    QMetaObject::Connection m_mocTerm;
    QMetaObject::Connection m_mocReady;

private slots:
    void handleTermRemoved(TermInstance *term);
    void handleTermChanged(TermInstance *term);

    void handleTermAdded(TermInstance *term);
    void handlePopulated();

    void forwardSplitRequest(int type);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    StackWidget(TermStack *stack, SplitBase *base);

    TermScrollport* currentScrollport() const;
    void showPeek();

    inline void setStateId(const Tsq::Uuid &id) { m_stateId = id; }
    inline void addStateRecord(const ScrollportState &state)
    { m_stateRecords[state.id] = state; }

    QSize calculateTermSize(const ProfileSettings *profile) const;
    QSize sizeHint() const;

    void saveLayout(SplitLayoutWriter &layout) const;

public slots:
    void takeFocus();
};
