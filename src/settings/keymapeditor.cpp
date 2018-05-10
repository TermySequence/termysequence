// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "app/selhelper.h"
#include "keymapeditor.h"
#include "rulemodel.h"
#include "keymap.h"
#include "keyinput.h"
#include "ruleeditor.h"
#include "global.h"
#include "state.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Load compiled-in default values?")
#define TR_BUTTON1 TL("input-button", "New Rule") + A("...")
#define TR_BUTTON2 TL("input-button", "New Child Rule") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Rule")
#define TR_BUTTON4 TL("input-button", "Edit Rule") + A("...")
#define TR_BUTTON5 TL("input-button", "Move to Top")
#define TR_BUTTON6 TL("input-button", "Move Up")
#define TR_BUTTON7 TL("input-button", "Move Down")
#define TR_BUTTON8 TL("input-button", "Move to Bottom")
#define TR_BUTTON9 TL("input-button", "Clear Search")
#define TR_BUTTON10 TL("input-button", "Keymap Options") + A("...")
#define TR_ERROR1 TL("error", "Please select a binding of type Digraph")
#define TR_FIELD1 TL("input-field", "Search") + ':'
#define TR_FIELD2 TL("input-field", "Enter Key or Button") + ':'
#define TR_TITLE1 TL("window-title", "Edit Keymap %1")
#define TR_TITLE2 TL("window-title", "Confirm Reset")

KeymapEditor::KeymapEditor(TermKeymap *keymap):
    m_keymap(keymap)
{
    setWindowTitle(TR_TITLE1.arg(keymap->name()));
    setAttribute(Qt::WA_QuitOnClose, false);

    m_model = new KeymapRuleModel(keymap, this);
    m_filter = new KeymapRuleFilter(m_model, this);
    m_view = new KeymapRuleView(m_filter);

    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_childButton = new IconButton(ICON_CHILD_ITEM, TR_BUTTON2);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON4);
    m_optionButton = new IconButton(ICON_CONFIGURE, TR_BUTTON10);

    m_firstButton = new IconButton(ICON_MOVE_TOP, TR_BUTTON5);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON6);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON7);
    m_lastButton = new IconButton(ICON_MOVE_BOTTOM, TR_BUTTON8);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|
        QDialogButtonBox::Reset|QDialogButtonBox::RestoreDefaults,
        Qt::Vertical);

    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_childButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_firstButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_lastButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_optionButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("keymap-editor");
    QPushButton *defaultsButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_resetButton = buttonBox->button(QDialogButtonBox::Reset);

    m_search = new QLineEdit;
    KeystrokeInput *keystroke = new KeystrokeInput;
    QPushButton *searchButton = new QPushButton(TR_BUTTON9);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(TR_FIELD1));
    searchLayout->addWidget(m_search, 2);
    searchLayout->addWidget(new QLabel(TR_FIELD2));
    searchLayout->addWidget(keystroke, 1);
    searchLayout->addWidget(searchButton);

    QVBoxLayout *viewLayout = new QVBoxLayout();
    viewLayout->addLayout(searchLayout);
    viewLayout->addWidget(m_view, 1);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addLayout(viewLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(m_filter, SIGNAL(modelReset()), SLOT(handleSelection()));
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(keymap, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(keymap, SIGNAL(rulesReloaded()), SLOT(handleReset()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewRule()));
    connect(m_childButton, SIGNAL(clicked()), SLOT(handleChildRule()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteRule()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditRule()));
    connect(m_applyButton, SIGNAL(clicked()), SLOT(handleApply()));
    connect(m_resetButton, SIGNAL(clicked()), SLOT(handleReset()));
    connect(defaultsButton, SIGNAL(clicked()), SLOT(handleDefaults()));
    connect(m_firstButton, SIGNAL(clicked()), SLOT(handleFirst()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleDown()));
    connect(m_lastButton, SIGNAL(clicked()), SLOT(handleLast()));
    connect(m_optionButton, SIGNAL(clicked()), SLOT(handleOptions()));

    connect(m_search, SIGNAL(textChanged(const QString&)), m_filter, SLOT(setSearchString(const QString&)));
    connect(keystroke, SIGNAL(keystrokeReceived(int)), m_filter, SLOT(setKeystroke(int)));
    connect(searchButton, SIGNAL(clicked()), SLOT(handleResetSearch()));

    handleSelection();
}

bool
KeymapEditor::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        if (!m_accepting)
            handleRejected();
        g_state->store(KeymapGeometryKey, saveGeometry());
        break;
    case QEvent::WindowActivate:
        m_view->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

void
KeymapEditor::bringUp()
{
    restoreGeometry(g_state->fetch(KeymapGeometryKey));

    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
    m_accepting = false;

    show();
    raise();
    activateWindow();
}

void
KeymapEditor::handleSelection()
{
    const OrderedKeymapRule *rule = m_view->selectedRule();

    if (!rule) {
        m_deleteButton->setEnabled(
            !m_view->selectionModel()->selectedRows(0).isEmpty());
        m_editButton->setEnabled(false);
        m_childButton->setEnabled(false);
        m_firstButton->setEnabled(false);
        m_upButton->setEnabled(false);
        m_downButton->setEnabled(false);
        m_lastButton->setEnabled(false);
    } else {
        m_deleteButton->setEnabled(true);
        m_editButton->setEnabled(true);
        m_childButton->setEnabled(rule->startsCombo);
        m_firstButton->setEnabled(rule->priority > 1);
        m_upButton->setEnabled(rule->priority > 1);
        m_downButton->setEnabled(rule->priority < rule->maxPriority);
        m_lastButton->setEnabled(rule->priority < rule->maxPriority);
    }
}

void
KeymapEditor::handleResetSearch()
{
    m_search->clear();

    if (m_filter->filtering())
        m_filter->resetSearch();

    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty())
        m_view->scrollTo(indexes.at(0));
}

void
KeymapEditor::handleNewRule()
{
    OrderedKeymapRule rule;
    RuleEditor dialog(rule, this);

    if (dialog.exec() == QDialog::Accepted) {
        const QModelIndex i = m_model->addRule(dialog.rule());
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j, QAbstractItemView::PositionAtCenter);
        handleChange();
    }
}

void
KeymapEditor::handleChildRule()
{
    OrderedKeymapRule *parentRule = m_view->selectedRule();
    QModelIndex index = m_view->selectedIndex();

    if (!parentRule || !parentRule->startsCombo) {
        errBox(TR_ERROR1, this)->show();
        return;
    }

    OrderedKeymapRule rule;
    rule.continuesCombo = true;
    RuleEditor dialog(rule, this);

    if (dialog.exec() == QDialog::Accepted) {
        const QModelIndex i = m_model->addRule(dialog.rule(), index);
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j, QAbstractItemView::PositionAtCenter);
        handleChange();
    }
}

void
KeymapEditor::handleEditRule()
{
    OrderedKeymapRule *rule = m_view->selectedRule();
    QModelIndex index = m_view->selectedIndex();

    if (!rule)
        return;

    RuleEditor dialog(*rule, this);

    if (dialog.exec() == QDialog::Accepted) {
        m_model->removeRule(index);
        const QModelIndex i = m_model->addRule(dialog.rule(), index.parent());
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        handleChange();
    }
}

static bool
RuleIndexComparator(const QModelIndex &a, const QModelIndex &b)
{
    bool apiv = a.parent().isValid();
    bool bpiv = b.parent().isValid();

    if (apiv != bpiv)
        return apiv > bpiv;
    else
        return a.row() > b.row();
}

void
KeymapEditor::handleDeleteRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    for (auto &index: indexes)
        index = m_filter->mapToSource(index);

    std::sort(indexes.begin(), indexes.end(), RuleIndexComparator);

    for (const auto &index: qAsConst(indexes))
        m_model->removeRule(index);

    handleChange();
    m_view->setFocus(Qt::OtherFocusReason);
}

void
KeymapEditor::handleChange()
{
    m_applyButton->setEnabled(true);
    m_resetButton->setEnabled(true);
}

void
KeymapEditor::handleDefaults()
{
    if (QMessageBox::Yes == askBox(TR_TITLE2, TR_ASK1, this)->exec()) {
        m_model->loadDefaultRules();
        handleChange();
    }
}

void
KeymapEditor::handleFirst()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        const QModelIndex i = m_model->moveFirst(m_filter->mapToSource(indexes.at(0)));
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j);
    }
}

void
KeymapEditor::handleUp()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        const QModelIndex i = m_model->moveUp(m_filter->mapToSource(indexes.at(0)));
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j);
    }
}

void
KeymapEditor::handleDown()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        const QModelIndex i = m_model->moveDown(m_filter->mapToSource(indexes.at(0)));
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j);
    }
}

void
KeymapEditor::handleLast()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        const QModelIndex i = m_model->moveLast(m_filter->mapToSource(indexes.at(0)));
        const QModelIndex j = m_filter->mapFromSource(i);
        doSelectIndex(m_view, j, true);
        m_view->scrollTo(j);
    }
}

void
KeymapEditor::handleOptions()
{
    KeymapOptions dialog(m_keymap, this);
    dialog.exec();
}

void
KeymapEditor::handleApply()
{
    m_keymap->setRules(m_model->rules());
    m_keymap->save();
    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
}

void
KeymapEditor::handleAccept()
{
    handleApply();
    m_accepting = true;
    close();
}

void
KeymapEditor::handleReset()
{
    m_model->loadRules(m_keymap);
    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
}

void
KeymapEditor::handleRejected()
{
    if (m_applyButton->isEnabled())
        handleReset();
}
