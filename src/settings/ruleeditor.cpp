// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/customaction.h"
#include "app/keys.h"
#include "app/messagebox.h"
#include "app/slotcombo.h"
#include "base/manager.h"
#include "ruleeditor.h"
#include "ruleeditmodel.h"
#include "keyinput.h"
#include "keymap.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#define TR_CHECK1 TL("input-checkbox", "Perform action") + ':'
#define TR_CHECK2 TL("input-checkbox", "Send input to terminal") + ':'
#define TR_CHECK3 TL("input-checkbox", "Begin key combination")
#define TR_ERROR1 TL("error", "Unrecognized action \"%1\"")
#define TR_FIELD1 TL("input-field", "Key or Button") + ':'
#define TR_FIELD2 TL("input-field", "Enter Key or Button") + ':'
#define TR_FIELD3 TL("input-field", "Description") + ':'
#define TR_TEXT1 TL("window-text", "Conditions")
#define TR_TEXT2 TL("window-text", "Outcome")
#define TR_TEXT3 TL("window-text", "Key or Button")
#define TR_TITLE1 TL("window-title", "Edit Keymap Binding")
#define TR_TITLE2 TL("window-title", "Edit Keymap Options")

#define TR_OPTION1 TL("KeymapFlagsModel", "Prepend escape to keypresses when %1 pressed")
#define TR_VALUE1 TL("KeymapFlagsModel", "Inherit/Default")
#define TR_VALUE2 TL("KeymapFlagsModel", "Enabled")
#define TR_VALUE3 TL("KeymapFlagsModel", "Disabled")

//
// Rule editor
//
RuleEditor::RuleEditor(const OrderedKeymapRule &rule, QWidget *parent):
    QDialog(parent),
    m_rule(rule)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    m_model = new KeymapFlagsModel(rule.conditions, rule.mask, this);
    m_view = new KeymapFlagsView(m_model);

    m_actionRadio = new QRadioButton(TR_CHECK1);
    m_inputRadio = new QRadioButton(TR_CHECK2);
    m_comboRadio = new QRadioButton(TR_CHECK3);

    m_keys = new QComboBox;
    KeystrokeInput *keystroke = new KeystrokeInput;
    m_input = new QLineEdit;
    m_action = new SlotCombo;

    populateLists();
    populateRule();

    QVBoxLayout *keyLayout = new QVBoxLayout;
    keyLayout->addWidget(new QLabel(TR_FIELD1));
    keyLayout->addWidget(m_keys);
    keyLayout->addWidget(new QLabel(TR_FIELD2));
    keyLayout->addWidget(keystroke);
    QGroupBox *keyGroup = new QGroupBox(TR_TEXT3);
    keyGroup->setLayout(keyLayout);

    QVBoxLayout *conditionLayout = new QVBoxLayout;
    conditionLayout->setContentsMargins(g_mtmargins);
    conditionLayout->addWidget(m_view);
    QGroupBox *conditionGroup = new QGroupBox(TR_TEXT1);
    conditionGroup->setLayout(conditionLayout);

    QVBoxLayout *actionLayout = new QVBoxLayout;
    actionLayout->addStretch();
    actionLayout->addWidget(m_actionRadio);
    actionLayout->addWidget(m_action);
    actionLayout->addStretch();
    actionLayout->addWidget(m_inputRadio);
    actionLayout->addWidget(m_input);
    actionLayout->addStretch();
    actionLayout->addWidget(m_comboRadio);
    actionLayout->addStretch();
    QGroupBox *actionGroup = new QGroupBox(TR_TEXT2);
    actionGroup->setLayout(actionLayout);

    QVBoxLayout *sideLayout = new QVBoxLayout;
    sideLayout->setContentsMargins(g_mtmargins);
    sideLayout->addWidget(keyGroup);
    sideLayout->addWidget(actionGroup);
    sideLayout->addStretch(1);

    QHBoxLayout *panelLayout = new QHBoxLayout;
    panelLayout->setContentsMargins(g_mtmargins);
    panelLayout->addWidget(conditionGroup, 1);
    panelLayout->addLayout(sideLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(panelLayout, 1);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

    connect(m_actionRadio, SIGNAL(toggled(bool)), SLOT(handleRadioChange()));
    connect(m_inputRadio, SIGNAL(toggled(bool)), SLOT(handleRadioChange()));
    connect(m_comboRadio, SIGNAL(toggled(bool)), SLOT(handleRadioChange()));
    connect(keystroke, SIGNAL(keystrokeReceived(int)), SLOT(handleKeystroke(int)));
}

void
RuleEditor::populateLists()
{
    for (auto &i: sortedKeyNames())
        m_keys->addItem(i, g_keyNames[i]);

    m_action->addItems(TermManager::allSlots());
    m_action->addItems(ActionFeature::customSlots());
}

void
RuleEditor::populateRule()
{
    // Populate values from rule
    m_keys->setCurrentText(m_rule.keyName);

    if (!m_rule.special) {
        m_inputRadio->setChecked(true);
        m_input->setText(m_rule.outcomeStr);
    } else
        m_input->setEnabled(false);

    if (m_rule.special && !m_rule.startsCombo) {
        m_actionRadio->setChecked(true);
        m_action->setCurrentText(m_rule.outcomeStr);
    } else
        m_action->setEnabled(false);

    if (m_rule.continuesCombo)
        m_comboRadio->setEnabled(false);
    else if (m_rule.startsCombo) {
        m_comboRadio->setChecked(true);
        m_inputRadio->setEnabled(false);
        m_input->setEnabled(false);
        m_actionRadio->setEnabled(false);
        m_action->setEnabled(false);
    }
}

void
RuleEditor::handleRadioChange()
{
    m_action->setEnabled(m_actionRadio->isChecked());
    m_input->setEnabled(m_inputRadio->isChecked());
}

void
RuleEditor::handleAccept()
{
    m_rule.conditions = m_model->conditions();
    m_rule.mask = m_model->mask();
    m_rule.key = g_keyNames[m_keys->currentText()];
    m_rule.keyName = m_keys->currentText();

    if (m_comboRadio->isChecked()) {
        m_rule.special = true;
        m_rule.startsCombo = true;
        m_rule.outcome = B("BeginDigraph");
    } else if (m_actionRadio->isChecked()) {
        m_rule.special = true;
        m_rule.startsCombo = false;
        m_rule.outcome = m_action->currentText().toUtf8();
        if (!TermManager::validateSlot(m_rule.outcome)) {
            errBox(TR_ERROR1.arg(m_action->currentText()), this)->show();
            return;
        }
    } else {
        QString spec = m_input->text();
        if (!spec.startsWith('"'))
            spec.prepend('"');
        if (!spec.endsWith('"'))
            spec.append('"');
        m_rule.special = false;
        m_rule.startsCombo = false;
        m_rule.outcome = KeymapRule::parseBytes(spec.mid(1, spec.size() - 2));
    }

    m_rule.populateStrings();
    accept();
}

void
RuleEditor::handleKeystroke(int key)
{
    int idx = m_keys->findData(key);
    m_keys->setCurrentIndex(idx != -1 ? idx : 0);
}

//
// Keymap options
//
KeymapOptions::KeymapOptions(TermKeymap *keymap, QWidget *parent) :
    QDialog(parent),
    m_keymap(keymap)
{
    setWindowTitle(TR_TITLE2);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    m_text = new QLineEdit(keymap->description());
    unsigned disabled = keymap->disabledFeatures();
    unsigned enabled = keymap->enabledFeatures();

    QStringList values({ TR_VALUE1, TR_VALUE2, TR_VALUE3 });

    m_alt = new QComboBox;
    m_alt->addItems(values);
    m_meta = new QComboBox;
    m_meta->addItems(values);

    if (disabled & Tsqt::Alt)
        m_alt->setCurrentIndex(2);
    else if (enabled & Tsqt::Alt)
        m_alt->setCurrentIndex(1);

    if (disabled & Tsqt::Meta)
        m_meta->setCurrentIndex(2);
    else if (enabled & Tsqt::Meta)
        m_meta->setCurrentIndex(1);

    QString alt = TL("KeymapFlagsModel", KeymapRule::getFlagName(RULEFLAG_ALT));
    QString meta = TL("KeymapFlagsModel", KeymapRule::getFlagName(RULEFLAG_META));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(TR_FIELD3), 0, 0);
    layout->addWidget(m_text, 0, 1, 1, 2);
    layout->addWidget(new QLabel(TR_OPTION1.arg(alt) + ':'), 1, 0, 1, 2);
    layout->addWidget(m_alt, 1, 2);
    layout->addWidget(new QLabel(TR_OPTION1.arg(meta) + ':'), 2, 0, 1, 2);
    layout->addWidget(m_meta, 2, 2);
    layout->setColumnStretch(1, 1);
    layout->addWidget(buttonBox, 3, 0, 1, 3);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
KeymapOptions::handleAccept()
{
    unsigned enabled = (m_alt->currentIndex() == 1 ? Tsqt::Alt : 0) |
        (m_meta->currentIndex() == 1 ? Tsqt::Meta : 0);
    unsigned disabled = (m_alt->currentIndex() == 2 ? Tsqt::Alt : 0) |
        (m_meta->currentIndex() == 2 ? Tsqt::Meta : 0);

    m_keymap->setFeatures(enabled, disabled);
    // Validate text
    m_keymap->setDescription(m_text->text().remove('"'));
    m_keymap->save();
    accept();
}
