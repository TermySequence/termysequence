// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "colorstab.h"
#include "colorsmodel.h"

#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QRegularExpression>

#define TR_BUTTON1 TL("input-button", "Select Color") + A("...")
#define TR_CHECK1 TL("input-checkbox", "Enable this color with a value of") + ':'
#define TR_FIELD1 TL("input-field", "Index") + ':'

ColorsTab::ColorsTab(Termcolors &tcpal) :
    m_tcpal(tcpal)
{
    m_model = new ColorsModel(tcpal, this);
    m_view = new ColorsView(m_model);

    m_spin = new QSpinBox;
    m_spin->setRange(PALETTE_APP + 4, PALETTE_APP + PALETTE_APP_SIZE - 1);
    m_text = new QLineEdit;
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_check = new QCheckBox(TR_CHECK1);

    m_button = new IconButton(ICON_CHOOSE_COLOR, TR_BUTTON1);

    QHBoxLayout *editLayout = new QHBoxLayout;
    editLayout->addWidget(new QLabel(TR_FIELD1));
    editLayout->addWidget(m_spin);
    editLayout->addWidget(m_check);
    editLayout->addWidget(m_text, 1);
    editLayout->addWidget(m_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_view, 1);
    layout->addLayout(editLayout);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    m_mocSpin = connect(m_spin, SIGNAL(valueChanged(int)), SLOT(handleValueChanged(int)));
    connect(m_text, SIGNAL(editingFinished()), SLOT(handleTextEdited()));
    connect(m_check, SIGNAL(toggled(bool)), SLOT(handleCheckBox(bool)));
    connect(m_button, SIGNAL(clicked()), SLOT(handleColorSelect()));
}

void
ColorsTab::reload()
{
    m_model->reloadData();
    handleSelection();
}

void
ColorsTab::bringUp()
{
    m_view->selectRow(0);
}

void
ColorsTab::handleSelection()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows();

    if (!indexes.isEmpty()) {
        int pos = m_model->row2pos(indexes.at(0).row());
        QRgb value = m_tcpal[pos];
        m_spin->setValue(pos);
        m_text->setText(QColor(value).name(QColor::HexRgb));
        m_check->setChecked(PALETTE_IS_ENABLED(value));
    }
}

void
ColorsTab::handleValueChanged(int v)
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows();
    int pos = -1;

    disconnect(m_mocSpin);

    if (v == PALETTE_APP_BG + 1)
        m_spin->setValue(v = PALETTE_APP_HANDLE_OFF);
    else if (v > PALETTE_APP_BG && v < PALETTE_APP_HANDLE_OFF)
        m_spin->setValue(v = PALETTE_APP_BG);

    if (!indexes.isEmpty())
        pos = m_model->row2pos(indexes.at(0).row());
    if (pos != v)
        m_view->selectRow(m_model->pos2row(v));

    m_mocSpin = connect(m_spin, SIGNAL(valueChanged(int)), SLOT(handleValueChanged(int)));
}

void
ColorsTab::setCurrentColor(const QColor &color)
{
    int v = m_spin->value();

    QRgb value = color.rgb() & PALETTE_COLOR;

    if (!m_check->isChecked())
        value |= PALETTE_DISABLED;

    if (m_tcpal[v] != value) {
        m_tcpal[v] = value;
        m_text->setText(QColor(value).name(QColor::HexRgb));

        m_model->reloadOne(v);
        emit modified();
    }
}

void
ColorsTab::handleTextEdited()
{
    QString trimmed = m_text->text().trimmed();

    // Try to put a hash mark on the front of hex chars
    QRegularExpression hexRe(L("\\A[0-9A-Fa-f]+\\z"));
    if (hexRe.match(trimmed).hasMatch())
        trimmed.prepend('#');

    QColor tmp(trimmed);
    if (tmp.isValid())
        setCurrentColor(tmp);
}

void
ColorsTab::handleColorSelect()
{
    QColor saved = m_tcpal[m_spin->value()];

    auto *dialog = colorBox(saved, this);
    connect(dialog, &QColorDialog::currentColorChanged, this, &ColorsTab::setCurrentColor);
    connect(dialog, &QDialog::rejected, [=]{ setCurrentColor(saved); });
    dialog->show();
}

void
ColorsTab::handleCheckBox(bool checked)
{
    m_text->setEnabled(checked);
    m_button->setEnabled(checked);

    setCurrentColor(QColor(m_tcpal[m_spin->value()]));
}
