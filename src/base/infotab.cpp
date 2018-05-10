// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "infotab.h"
#include "infoattrmodel.h"
#include "idbase.h"

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Submit")
#define TR_CHECK1 TL("input-checkbox", "Delete")
#define TR_FIELD1 TL("input-field", "Key") + ':'
#define TR_FIELD2 TL("input-field", "Value") + ':'

InfoTab::InfoTab(IdBase *subject)
{
    m_model = new InfoAttrModel(subject->attributes(), this);
    auto *view = new InfoAttrView(m_model);

    m_key = new QLineEdit;
    m_value = new QLineEdit;
    m_check = new QCheckBox(TR_CHECK1);
    m_button = new IconButton(ICON_SUBMIT_ITEM, TR_BUTTON1);

    QHBoxLayout *editLayout = new QHBoxLayout;
    editLayout->addWidget(new QLabel(TR_FIELD1));
    editLayout->addWidget(m_key, 1);
    editLayout->addWidget(new QLabel(TR_FIELD2));
    editLayout->addWidget(m_value, 1);
    editLayout->addWidget(m_check);
    editLayout->addWidget(m_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(view, 1);
    layout->addLayout(editLayout);
    setLayout(layout);

    connect(view->selectionModel(),
            SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
            SLOT(handleSelection(const QModelIndex&)));

    connect(m_check, SIGNAL(toggled(bool)), SLOT(handleCheckBox(bool)));
    connect(m_button, SIGNAL(clicked()), SIGNAL(changeSubmitted()));

    connect(subject,
            SIGNAL(attributeChanged(const QString&,const QString&)),
            m_model,
            SLOT(handleAttributeChanged(const QString&,const QString&)));
    connect(subject,
            SIGNAL(attributeRemoved(const QString&)),
            m_model,
            SLOT(handleAttributeRemoved(const QString&)));
}

void
InfoTab::handleSelection(const QModelIndex &index)
{
    if (index.isValid()) {
        QString key, value;
        m_model->data(index.row(), key, value);

        m_key->setText(key);
        m_value->setText(value);
        m_check->setChecked(false);
    }
}

void
InfoTab::handleCheckBox(bool checked)
{
    m_value->setEnabled(!checked);
}

QString
InfoTab::key() const
{
    return m_key->text();
}

QString
InfoTab::value() const
{
    return m_value->text();
}

bool
InfoTab::removal() const
{
    return m_check->isChecked();
}

void
InfoTab::showEvent(QShowEvent *event)
{
    m_model->setVisible(true);
    QWidget::showEvent(event);
}

void
InfoTab::hideEvent(QHideEvent *event)
{
    m_model->setVisible(false);
    QWidget::hideEvent(event);
}
