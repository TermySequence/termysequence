// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "encodingdialog.h"
#include "base.h"

#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QEvent>

#define TR_TEXT1 TL("window-text", "Specify encoding parameters, one per line")
#define TR_TITLE1 TL("window-title", "Configure Encoding")

EncodingDialog::EncodingDialog(const SettingDef *def, SettingsBase *settings,
                               QWidget *parent) :
    QDialog(parent),
    m_def(def),
    m_settings(settings)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_params = new QPlainTextEdit;
    m_params->setLineWrapMode(QPlainTextEdit::NoWrap);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_TEXT1));
    layout->addWidget(m_params);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

bool
EncodingDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_params->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
EncodingDialog::setContent(const QStringList &params)
{
    m_variant = params.front();
    m_params->clear();

    for (int i = 1, n = params.size(); i < n; ++i)
        if (!params[i].isEmpty())
            m_params->appendPlainText(params[i]);
}

void
EncodingDialog::handleAccept()
{
    QStringList params(m_variant);
    QString content = m_params->toPlainText();
    params += content.split('\n', QString::SkipEmptyParts);

    m_settings->setProperty(m_def->property, params);
    accept();
}
