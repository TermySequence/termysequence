// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "filleditor.h"
#include "lib/palette.h"

#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QPainter>

#define TR_DIM0 TL("settings-dimension", "Foreground", "value")
#define TR_FIELD1 TL("input-field", "Column") + ':'
#define TR_FIELD2 TL("input-field", "Color") + ':'
#define TR_FIELD3 TL("input-field", "Preview") + ':'
#define TR_TITLE1 TL("window-title", "New Column Fill")
#define TR_TITLE2 TL("window-title", "Edit Column Fill")

//
// Dialog
//
FillEditor::FillEditor(QWidget *parent, const Termcolors &tcpal,
                       const QFont &font, bool adding):
    QDialog(parent)
{
    setWindowTitle(adding ? TR_TITLE1 : TR_TITLE2);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    m_column = new QSpinBox;
    m_column->setRange(0, 512);
    m_column->setValue(80);
    m_color = new QSpinBox;
    m_color->setRange(-1, PALETTE_APP - 1);
    m_color->setValue(-1);
    m_color->setSpecialValueText(TR_DIM0);
    m_preview = new FillPreview(tcpal, font);
    m_preview->setColorIndex(-1);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(TR_FIELD1), 0, 0);
    layout->addWidget(m_column, 0, 1);
    layout->addWidget(new QLabel(TR_FIELD2), 1, 0);
    layout->addWidget(m_color, 1, 1);
    layout->addWidget(new QLabel(TR_FIELD3), 2, 0);
    layout->addWidget(m_preview, 2, 1);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(2, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(layout, 1);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(m_color, SIGNAL(valueChanged(int)), m_preview, SLOT(setColorIndex(int)));
    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

TermLayout::Fill
FillEditor::fill() const
{
    int value = m_color->value();
    QRgb color = (value != -1) ? value : PALETTE_DISABLED;
    return TermLayout::Fill(m_column->value(), color);
}

void
FillEditor::setFill(const TermLayout::Fill &fill)
{
    m_column->setValue(fill.first);

    int value = PALETTE_IS_ENABLED(fill.second) ? fill.second : -1;
    m_color->setValue(value);
    m_preview->setColorIndex(value);
}

//
// Preview
//
FillPreview::FillPreview(const Termcolors &tcpal, const QFont &font) :
    m_tcpal(tcpal),
    m_height(QFontMetrics(font).height()),
    m_line(1 + m_height / MARGIN_INCREMENT)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
}

void
FillPreview::setColorIndex(int index)
{
    m_index = index;
    update();
}

void
FillPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_tcpal.bg());

    QColor color(m_tcpal.at(m_index != -1 ? m_index : PALETTE_APP_FG));
    color.setAlphaF(0.4);

    QPen pen;
    pen.setWidth(m_line);
    pen.setColor(color);
    painter.setPen(pen);
    int half = height() / 2;
    painter.drawLine(0, half, width(), half);
}

QSize
FillPreview::sizeHint() const
{
    return QSize(1, m_height * 2);
}
