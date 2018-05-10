// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "dockwidget.h"
#include "lib/uuid.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE
class NoteModel;
class NoteFilter;
class NoteView;
struct TermNote;

class NoteWidget final: public SearchableWidget
{
    Q_OBJECT

private:
    NoteFilter *m_filter;
    NoteView *m_view;

    QCheckBox *m_check;

    const TermNote* selectedNote() const;

public:
    NoteWidget(TermManager *manager, NoteModel *model);

    TermInstance* selectedTerm() const;
    regionid_t selectedRegion() const;

    void toolAction(int index);
    void contextMenu();

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    void setWhitelist(const Tsq::Uuid &id);
    void addWhitelist(const Tsq::Uuid &id);
    void emptyWhitelist();
    void addBlacklist(const Tsq::Uuid &id);
    void resetFilter();

    void removeClosedTerminals();

    void restoreState(int index);
    void saveState(int index);
};
