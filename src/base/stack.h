// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>

class TermInstance;
class TermManager;
class TermScrollport;
class StackWidget;
class ProfileSettings;

class TermStack final: public QObject
{
    Q_OBJECT

private:
    TermManager *m_manager;
    StackWidget *m_parent;

    TermInstance *m_term = nullptr;
    int m_index = 0;
    int m_pos = 0;

signals:
    void termChanged(TermInstance *term);
    void indexChanged(int index);

    void splitRequest(int type);
    void focusRequest();

private slots:
    void handleTermRemoved(TermInstance *term, TermInstance *replacement);

public:
    TermStack(TermManager *manager);
    ~TermStack();
    void setParent(StackWidget *widget);

    inline TermManager* manager() { return m_manager; }
    inline TermInstance* term() { return m_term; }
    inline int index() const { return m_index; }
    inline int pos() const { return m_pos; }

    TermScrollport* currentScrollport() const;
    void showPeek();

    bool setTerm(TermInstance *term);
    void setIndex(int index);
    inline void setPos(int pos) { m_pos = pos; }

    QSize calculateTermSize(const ProfileSettings *profile) const;
};

#define SPLITREQ_HRESIZE     0
#define SPLITREQ_VRESIZE     1
#define SPLITREQ_HFIXED      2
#define SPLITREQ_VFIXED      3
#define SPLITREQ_QFIXED      4
#define SPLITREQ_CLOSE       5
#define SPLITREQ_CLOSEOTHERS 6
#define SPLITREQ_EXPAND      7
#define SPLITREQ_SHRINK      8
#define SPLITREQ_EQUALIZEALL 9
#define SPLITREQ_EQUALIZE    10
