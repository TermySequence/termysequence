// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "base/thumbicon.h"
#include "base/idbase.h"
#include "imagedialog.h"
#include "imagemodel.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>

#define TR_FIELD1 TL("input-field", "Search") + ':'
#define TR_FIELD2 TL("input-field", "Add SVG images to") + A(": ")
#define TR_TITLE1 TL("window-title", "Select Icon")

void
ImageDialog::setup(int iconType, bool resettable)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_model = new ImageModel(iconType, this);
    m_view = new ImageView(m_model);
    QLineEdit *text = new QLineEdit;
    text->setClearButtonEnabled(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    QHBoxLayout *lineLayout = new QHBoxLayout;
    lineLayout->addWidget(new QLabel(TR_FIELD1));
    lineLayout->addWidget(text, 1);

    QString path = ThumbIcon::getPath((ThumbIcon::IconType)iconType);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_view, 1);
    layout->addLayout(lineLayout);
    if (!path.isEmpty()) {
        auto *label = new QLabel(TR_FIELD2 + path);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }
    layout->addWidget(buttonBox);
    setLayout(layout);

    if (resettable) {
        auto *resetButton = buttonBox->addButton(QDialogButtonBox::Reset);
        connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
    }

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccepted()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

    connect(text, SIGNAL(textChanged(const QString&)),
            SLOT(handleTextChanged(const QString&)));
    connect(m_model, SIGNAL(modelReset()), SLOT(handleModelReset()));

    m_view->resizeRowsToContents();
}

ImageDialog::ImageDialog(int type, bool resettable, QWidget *parent) :
    QDialog(parent)
{
    setup(type, resettable);
}

ImageDialog::ImageDialog(int type, IdBase *idbase, QWidget *parent) :
    QDialog(parent),
    m_idStr(idbase->idStr())
{
    setup(type, true);
    setCurrentIcon(idbase->icons()[1]);

    connect(idbase, SIGNAL(destroyed()), SLOT(deleteLater()));
}

void
ImageDialog::setCurrentIcon(const QString &icon)
{
    m_view->setSelectedImage(icon);
}

void
ImageDialog::handleTextChanged(const QString &text)
{
    m_model->setSearchString(text);
    m_view->setSelectedImage(QString());
}

void
ImageDialog::handleModelReset()
{
    m_view->resizeRowsToContents();
}

void
ImageDialog::handleReset()
{
    m_view->clearSelectedImage();
    accept();
}

void
ImageDialog::handleAccepted()
{
    emit okayed(m_view->selectedImage(), m_idStr);
    accept();
}

bool
ImageDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_view->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}
