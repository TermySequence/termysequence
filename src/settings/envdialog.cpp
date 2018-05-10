// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "envdialog.h"
#include "settings.h"
#include "profile.h"
#include "lib/attr.h"

#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QEvent>

#define TR_TAB1 TL("tab-title", "Assign")
#define TR_TAB2 TL("tab-title", "Remove")
#define TR_TAB3 TL("tab-title", "Answerback")
#define TR_TEXT1 TL("window-text", "Specify name=value rules, one per line")
#define TR_TEXT2 TL("window-text", "Specify names to remove, one per line")
#define TR_TEXT3 TL("window-text", "Specify answerback response string")
#define TR_TITLE1 TL("window-title", "Set Environment")

EnvironDialog::EnvironDialog(const SettingDef *def, SettingsBase *settings, QWidget *parent) :
    QDialog(parent),
    m_def(def),
    m_settings(settings)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_tabs = new QTabWidget;
    m_set = new QPlainTextEdit;
    m_set->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_clear = new QPlainTextEdit;
    m_clear->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_answerback = new QLineEdit;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_TEXT1));
    layout->addWidget(m_set);
    QWidget *page = new QWidget;
    page->setLayout(layout);
    m_tabs->addTab(page, TR_TAB1);

    layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_TEXT2));
    layout->addWidget(m_clear);
    page = new QWidget;
    page->setLayout(layout);
    m_tabs->addTab(page, TR_TAB2);

    layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_TEXT3));
    layout->addWidget(m_answerback);
    layout->addStretch(1);
    page = new QWidget;
    page->setLayout(layout);
    m_tabs->addTab(page, TR_TAB3);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults);

    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);

    layout = new QVBoxLayout;
    layout->addWidget(m_tabs);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(handleDefaults()));
}

bool
EnvironDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_tabs->setCurrentIndex(0);
        m_set->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
EnvironDialog::setContent(const QStringList &rules)
{
    m_set->clear();
    m_clear->clear();

    for (auto &i: rules) {
        if (i.isEmpty())
            continue;

        switch (i.at(0).unicode()) {
        case '+':
            m_set->appendPlainText(i.mid(1));
            break;
        case '-':
            m_clear->appendPlainText(i.mid(1));
            break;
        case '*':
            if (i.startsWith(A(TSQ_ENV_ANSWERBACK)))
                m_answerback->setText(i.mid(sizeof(TSQ_ENV_ANSWERBACK) - 1));
            break;
        }
    }
}

void
EnvironDialog::handleDefaults()
{
    setContent(m_settings->defaultValue(m_def).toStringList());
}

void
EnvironDialog::handleAccept()
{
    QString content = m_set->toPlainText();
    QStringList rules = content.split('\n', QString::SkipEmptyParts);
    for (auto &i: rules)
        i.prepend('+');

    content = m_clear->toPlainText();
    QStringList clears = content.split('\n', QString::SkipEmptyParts);
    for (auto &i: clears)
        rules.append('-' + i);

    QString answerback = m_answerback->text();
    if (!answerback.isEmpty())
        rules.append(A(TSQ_ENV_ANSWERBACK) + answerback);

    m_settings->setProperty(m_def->property, rules);
    accept();
}
