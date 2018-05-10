// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "sizewidget.h"

#include <QSpinBox>
#include <QHBoxLayout>
#include <QLabel>

#define TR_FIELD1 TL("input-field", "Columns") + ':'
#define TR_FIELD2 TL("input-field", "Rows") + ':'

SizeWidget::SizeWidget(const SettingDef *def, SettingsBase *settings) :
    SettingWidget(def, settings)
{
    m_cols = new QSpinBox;
    m_cols->installEventFilter(this);
    m_cols->setRange(TERM_MIN_COLS, TERM_MAX_COLS);
    m_rows = new QSpinBox;
    m_rows->installEventFilter(this);
    m_rows->setRange(TERM_MIN_ROWS, TERM_MAX_ROWS);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_cols);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_rows);
    layout->addStretch(1);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_cols, SIGNAL(valueChanged(int)), this, SLOT(handleColsChanged(int)));
    connect(m_rows, SIGNAL(valueChanged(int)), this, SLOT(handleRowsChanged(int)));
}

void
SizeWidget::handleColsChanged(int cols)
{
    m_size.setWidth(cols);
    setProperty(m_size);
}

void
SizeWidget::handleRowsChanged(int rows)
{
    m_size.setHeight(rows);
    setProperty(m_size);
}

void
SizeWidget::handleSettingChanged(const QVariant &value)
{
    m_size = value.toSize();
    m_cols->setValue(m_size.width());
    m_rows->setValue(m_size.height());
}


QWidget *
SizeWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new SizeWidget(def, settings);
}

QString
SizeWidgetFactory::toString(const QVariant &value) const
{
    QSize size = value.toSize();
    return L("%1,%2").arg(size.width()).arg(size.height());
}

QVariant
SizeWidgetFactory::fromString(const QString &str) const
{
    QStringList list = str.split((QChar)',');
    QSize size;

    size.setWidth(list.at(0).toUInt());
    size.setHeight(list.value(1, A("0")).toUInt());
    return size;
}
