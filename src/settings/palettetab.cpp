// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "app/selhelper.h"
#include "palettetab.h"
#include "palettemodel.h"

#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QRegularExpression>

#define TR_BUTTON1 TL("input-button", "Select Color") + A("...")
#define TR_FIELD1 TL("input-field", "Index") + ':'
#define TR_FIELD2 TL("input-field", "Value") + ':'

PaletteTab::PaletteTab(Termcolors &tcpal) :
    m_tcpal(tcpal)
{
    m_model = new PaletteModel(m_tcpal, this);
    m_view = new PaletteView(m_model);

    m_spin = new QSpinBox;
    m_spin->setRange(0, PALETTE_STANDARD_MASK);
    m_text = new QLineEdit;
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QPushButton *selectButton = new IconButton(ICON_CHOOSE_COLOR, TR_BUTTON1);

    QHBoxLayout *editLayout = new QHBoxLayout;
    editLayout->addWidget(new QLabel(TR_FIELD1));
    editLayout->addWidget(m_spin);
    editLayout->addWidget(new QLabel(TR_FIELD2));
    editLayout->addWidget(m_text, 1);
    editLayout->addWidget(selectButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_view, 1);
    layout->addLayout(editLayout);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(m_spin, SIGNAL(valueChanged(int)), SLOT(handleValueChanged(int)));
    connect(m_text, SIGNAL(editingFinished()), SLOT(handleTextEdited()));
    connect(selectButton, SIGNAL(clicked()), SLOT(handleColorSelect()));
}

void
PaletteTab::reload()
{
    m_model->reloadData();
    handleSelection();
}

void
PaletteTab::bringUp()
{
    doSelectIndex(m_view, m_model->index(0, 0), false);
}

void
PaletteTab::handleSelection()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();

    if (!indexes.isEmpty()) {
        const QModelIndex &index = indexes.at(0);
        m_spin->setValue(index.row() * PALETTE_N_COLUMNS + index.column());
        QRgb value = index.data(Qt::UserRole).toUInt();
        m_text->setText(QColor(value).name(QColor::HexRgb));
    }
}

void
PaletteTab::handleValueChanged(int v)
{
    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    int pos = -1;

    if (!indexes.isEmpty()) {
        const QModelIndex &index = indexes.at(0);
        pos = index.row() * PALETTE_N_COLUMNS + index.column();
    }
    if (pos != v)
        doSelectIndex(m_view,
                      m_model->index(v / PALETTE_N_COLUMNS, v % PALETTE_N_COLUMNS),
                      false);
}

void
PaletteTab::setCurrentColor(const QColor &color)
{
    int v = m_spin->value();
    QRgb value = color.rgb() & PALETTE_COLOR;

    if (m_tcpal[v] != value) {
        m_tcpal[v] = value;
        m_text->setText(QColor(value).name(QColor::HexRgb));

        m_model->reloadOne(v);
        emit modified();
    }
}

void
PaletteTab::handleTextEdited()
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
PaletteTab::handleColorSelect()
{
    QColor saved = m_tcpal[m_spin->value()];

    auto *dialog = colorBox(saved, this);
    connect(dialog, &QColorDialog::currentColorChanged, this, &PaletteTab::setCurrentColor);
    connect(dialog, &QDialog::rejected, [=]{ setCurrentColor(saved); });
    dialog->show();
}
