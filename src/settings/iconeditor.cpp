// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "app/selhelper.h"
#include "base/thumbicon.h"
#include "iconeditor.h"
#include "iconmodel.h"
#include "iconrule.h"
#include "imagedialog.h"
#include "global.h"
#include "state.h"

#include <QMap>
#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Load compiled-in default values?")
#define TR_BUTTON1 TL("input-button", "Insert Rule")
#define TR_BUTTON2 TL("input-button", "Prepend Rule")
#define TR_BUTTON3 TL("input-button", "Append Rule")
#define TR_BUTTON4 TL("input-button", "Delete Rule")
#define TR_BUTTON5 TL("input-button", "Move to Top")
#define TR_BUTTON6 TL("input-button", "Move Up")
#define TR_BUTTON7 TL("input-button", "Move Down")
#define TR_BUTTON8 TL("input-button", "Move to Bottom")
#define TR_BUTTON9 TL("input-button", "Clone Rule")
#define TR_BUTTON10 TL("input-button", "Choose Icon") + A("...")
#define TR_ERROR2 TL("error", "Rule %1 is invalid or missing required fields")
#define TR_TITLE1 TL("window-title", "Edit Icon Ruleset")
#define TR_TITLE2 TL("window-title", "Confirm Reset")

IconEditor *g_iconwin;

IconEditor::IconEditor(IconRuleset *ruleset):
    m_ruleset(ruleset)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_model = new IconRuleModel(ruleset, this);
    m_view = new IconRuleView(m_model);

    m_insertButton = new IconButton(ICON_INSERT_ITEM, TR_BUTTON1);
    m_prependButton = new IconButton(ICON_PREPEND_ITEM, TR_BUTTON2);
    m_appendButton = new IconButton(ICON_APPEND_ITEM, TR_BUTTON3);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON4);
    m_cloneButton = new IconButton(ICON_CLONE_ITEM, TR_BUTTON9);
    m_iconButton = new IconButton(ICON_CHOOSE_ICON, TR_BUTTON10);

    m_firstButton = new IconButton(ICON_MOVE_TOP, TR_BUTTON5);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON6);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON7);
    m_lastButton = new IconButton(ICON_MOVE_BOTTOM, TR_BUTTON8);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|
        QDialogButtonBox::Reset|QDialogButtonBox::RestoreDefaults,
        Qt::Vertical);

    buttonBox->addButton(m_prependButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_insertButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_appendButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_iconButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_firstButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_lastButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("icon-rule-editor");
    QPushButton *defaultsButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_resetButton = buttonBox->button(QDialogButtonBox::Reset);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_view, 1);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(m_model, &IconRuleModel::dataChanged, this, &IconEditor::handleChange);
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_insertButton, SIGNAL(clicked()), SLOT(handleInsertRule()));
    connect(m_prependButton, SIGNAL(clicked()), SLOT(handlePrependRule()));
    connect(m_appendButton, SIGNAL(clicked()), SLOT(handleAppendRule()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneRule()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteRule()));
    connect(m_applyButton, SIGNAL(clicked()), SLOT(handleApply()));
    connect(m_iconButton, SIGNAL(clicked()), SLOT(handleIcon()));
    connect(m_firstButton, SIGNAL(clicked()), SLOT(handleFirst()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleDown()));
    connect(m_lastButton, SIGNAL(clicked()), SLOT(handleLast()));
    connect(m_resetButton, SIGNAL(clicked()), SLOT(handleReset()));
    connect(defaultsButton, SIGNAL(clicked()), SLOT(handleDefaults()));

    handleSelection();
}

bool
IconEditor::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        if (!m_accepting)
            handleRejected();
        g_state->store(IconGeometryKey, saveGeometry());
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
IconEditor::bringUp()
{
    m_model->resetIcons();

    restoreGeometry(g_state->fetch(IconGeometryKey));

    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
    m_accepting = false;

    show();
    raise();
    activateWindow();
}

void
IconEditor::handleSelection()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    bool enabled = !indexes.isEmpty();

    m_cloneButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled);
    m_firstButton->setEnabled(enabled);
    m_upButton->setEnabled(enabled);
    m_downButton->setEnabled(enabled);
    m_lastButton->setEnabled(enabled);
    m_iconButton->setEnabled(enabled);

    m_selLow = m_selHigh = -1;
    for (auto &i: qAsConst(indexes)) {
        int row = i.row();
        if (m_selLow == -1 || m_selLow > row)
            m_selLow = row;
        if (m_selHigh == -1 || m_selHigh < row)
            m_selHigh = row;
    }
}

void
IconEditor::handleChange()
{
    m_applyButton->setEnabled(true);
    m_resetButton->setEnabled(true);
}

void
IconEditor::handleInsertRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        int row = indexes.at(0).row();
        m_model->insertRule(row);
        m_view->selectRow(row);
        m_view->scrollTo(m_model->index(row, 0));
        handleChange();
    } else {
        handlePrependRule();
    }
}

void
IconEditor::handlePrependRule()
{
    m_model->insertRule(0);
    m_view->selectRow(0);
    m_view->scrollToTop();
    handleChange();
}

void
IconEditor::handleAppendRule()
{
    int row = m_model->rowCount();
    m_model->insertRule(row);
    m_view->selectRow(row);
    m_view->scrollToBottom();
    handleChange();
}

void
IconEditor::handleCloneRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    m_model->cloneRule(row);
    m_view->selectRow(++row);
    m_view->scrollTo(m_model->index(row, 0));
    handleChange();
}

void
IconEditor::handleDeleteRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    QMap<int,int> sorted;
    for (const auto &index: qAsConst(indexes))
        sorted[index.row()] = 1;

    int row = sorted.cbegin().key();
    for (auto i = sorted.cbegin(), j = sorted.cend(); i != j; ) {
        --j;
        m_model->removeRule(j.key());
    }

    int nrows = m_model->rowCount();
    if (row == nrows)
        --row;
    if (row >= 0) {
        m_view->selectRow(row);
        m_view->scrollTo(m_model->index(row, 0));
    }
    handleChange();
}

void
IconEditor::handleIcon()
{
    if (m_selLow == -1)
        return;

    auto *dialog = new ImageDialog(ThumbIcon::CommandType, false, this);
    dialog->setCurrentIcon(m_model->ruleset().at(m_selLow).toIcon);
    connect(dialog, &ImageDialog::okayed, [this](QString name) {
        m_model->setIcon(m_selLow, m_selHigh, name);
        handleChange();
    });
    dialog->show();
}

void
IconEditor::handleDefaults()
{
    if (QMessageBox::Yes == askBox(TR_TITLE2, TR_ASK1, this)->exec()) {
        m_model->loadDefaultRules();
        handleChange();
    }
}

void
IconEditor::handleFirst()
{
    if (m_selLow <= 0)
        return;

    m_model->moveFirst(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(0, 0);
    QModelIndex selEnd = m_model->index(m_selHigh - m_selLow, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollToTop();
}

void
IconEditor::handleUp()
{
    if (m_selLow <= 0)
        return;

    m_model->moveUp(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(m_selLow - 1, 0);
    QModelIndex selEnd = m_model->index(m_selHigh - 1, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollTo(selStart);
}

void
IconEditor::handleDown()
{
    if (m_selHigh == -1 || m_selHigh == m_model->rowCount() - 1)
        return;

    m_model->moveDown(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(m_selLow + 1, 0);
    QModelIndex selEnd = m_model->index(m_selHigh + 1, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollTo(selEnd);
}

void
IconEditor::handleLast()
{
    int last = m_model->rowCount() - 1;

    if (m_selHigh == -1 || m_selHigh == last)
        return;

    m_model->moveLast(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(last - (m_selHigh - m_selLow), 0);
    QModelIndex selEnd = m_model->index(last, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollToBottom();
}

bool
IconEditor::save()
{
    int i = m_model->ruleset().validateRules();
    if (i != -1) {
        m_view->selectRow(i);
        errBox(TR_ERROR2.arg(i + 1), this)->show();
        return false;
    }

    m_ruleset->setRules(m_model->ruleset());
    m_ruleset->save();
    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
    emit g_global->iconRulesChanged();
    return true;
}

void
IconEditor::handleAccept()
{
    if (save()) {
        m_accepting = true;
        close();
    }
}

void
IconEditor::handleApply()
{
    save();
}

void
IconEditor::handleReset()
{
    if (m_applyButton->isEnabled()) {
        m_model->loadRules(m_ruleset);
        m_applyButton->setEnabled(false);
        m_resetButton->setEnabled(false);
    }
    m_view->reset();
}

void
IconEditor::handleRejected()
{
    if (m_applyButton->isEnabled()) {
        m_model->loadRules(m_ruleset);
        m_applyButton->setEnabled(false);
    }
}
