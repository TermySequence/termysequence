// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/format.h"
#include "app/iconbutton.h"
#include "formatdialog.h"
#include "formatwidget.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>

#define TR_BUTTON1 TL("input-button", "Insert")
#define TR_FIELD1 TL("input-field", "Format string") + ':'
#define TR_FIELD2 TL("input-field", "Commonly used variables") + ':'
#define TR_TITLE1 TL("window-title", "Define Format String")

FormatDialog::FormatDialog(const FormatDef *specs, const QString &defval, QWidget *parent):
    QDialog(parent),
    m_defval(defval)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_text = new QLineEdit;
    m_text->setMaxLength(256);

    m_combo = new QComboBox;
    populateCombo(specs);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults);
    QPushButton *insertButton = new IconButton(ICON_INSERT_ITEM, TR_BUTTON1);
    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);

    QHBoxLayout *comboLayout = new QHBoxLayout;
    comboLayout->addWidget(m_combo, 1);
    comboLayout->addWidget(insertButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_text);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addLayout(comboLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(insertButton, SIGNAL(clicked()), SLOT(handleInsert()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
}

void
FormatDialog::populateCombo(const FormatDef *specs)
{
    for (const FormatDef *spec = specs; spec->description; ++spec) {
        QString val = spec->variable;
        QString str = QCoreApplication::translate("settings-enum", spec->description);
        m_combo->addItem(L("%1 - %2").arg(val, str), val);
    }
}

bool
FormatDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_text->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

QString
FormatDialog::text() const
{
    return m_text->text();
}

void
FormatDialog::setText(const QString &text)
{
    m_text->setText(text);
}

void
FormatDialog::handleInsert()
{
    QString str = m_combo->currentData().toString();
    if (str.at(0) != '\\') {
        str.prepend(A("\\("));
        str.append(')');
    }
    m_text->insert(str);
}

void
FormatDialog::handleReset()
{
    m_text->setText(m_defval);
}
