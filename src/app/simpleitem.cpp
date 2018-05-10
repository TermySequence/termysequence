// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "config.h"
#include "simpleitem.h"

#include <QComboBox>
#include <QPainter>
#include <QApplication>
#include <QFontDatabase>
#include <QSvgRenderer>

#define LINE_WIDTH 4

//
// Combo Item
//
ComboBoxItemDelegate::ComboBoxItemDelegate(bool editable, QObject *parent):
    QStyledItemDelegate(parent),
    m_editable(editable)
{
}

QWidget *
ComboBoxItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto strings = index.data(Qt::UserRole).toStringList();
    auto icons = index.data(Qt::UserRole + 1).toList();

    QComboBox *combo = new QComboBox(parent);
    combo->setEditable(m_editable);
    combo->addItems(strings);

    int j = icons.size(), k = strings.size();
    for (int i = 0, n = (j < k) ? j : k; i < n; ++i)
        combo->setItemIcon(i, icons[i].value<QIcon>());

    return combo;
}

void
ComboBoxItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox*>(editor);
    combo->setCurrentIndex(index.data(Qt::EditRole).toInt());
}

void
ComboBoxItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox*>(editor);
    model->setData(index, combo->currentIndex(), Qt::EditRole);
}

//
// Color Item
//
ColorItemDelegate::ColorItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    m_font(QFontDatabase::systemFont(QFontDatabase::FixedFont))
{
}

static inline QColor
foregroundColor(QRgb value)
{
    int r = qRed(value), g = qGreen(value), b = qBlue(value);
    return (r + g + b < 416) ? Qt::white : Qt::black;
}

void
ColorItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle::State flags = (QStyle::State_MouseOver|QStyle::State_Selected|QStyle::State_HasFocus);

    if (option.state & flags) {
        QStyleOptionViewItem opt = option;
        opt.state &= ~flags;
        QStyledItemDelegate::paint(painter, opt, index);

        QRgb value = index.data(Qt::UserRole).toUInt();
        QColor bg(value);

        QPen pen = painter->pen();
        pen.setBrush(foregroundColor(value));
        pen.setWidth(LINE_WIDTH);

        painter->save();
        painter->setPen(pen);
        painter->setClipRect(option.rect);
        painter->drawRect(option.rect);

        painter->setFont(m_font);
        Qt::Alignment align = Qt::AlignCenter;
        painter->drawText(option.rect, bg.name(QColor::HexRgb), align);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

//
// Radio Item
//
RadioButtonItemDelegate::RadioButtonItemDelegate(QObject *parent, bool isRadio) :
    QStyledItemDelegate(parent),
    m_isRadio(isRadio)
{
}

void
RadioButtonItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int data = index.data(Qt::UserRole).toInt();

    if (data == -1) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    QStyle *style = QApplication::style();

    int radioWidth = style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
    int radioHeight = style->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
    int checkWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, &option);
    int checkHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, &option);
    int checkSpacing = style->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &option);

    int x, y, w, h;
    QStyle::PrimitiveElement element;

    if (m_isRadio) {
        x = checkSpacing / 2;
        y = (opt.rect.height() - radioHeight) / 2;
        w = radioWidth;
        h = radioHeight;
        element = QStyle::PE_IndicatorRadioButton;
    } else {
        x = checkSpacing / 2;
        y = (opt.rect.height() - checkHeight) / 2;
        w = checkWidth;
        h = checkHeight;
        element = QStyle::PE_IndicatorCheckBox;
    }

    opt.rect.translate(x, y);
    opt.rect.setWidth(w);
    opt.rect.setHeight(h);
    opt.state |= (data == 1) ? QStyle::State_On : QStyle::State_Off;

    QStyledItemDelegate::paint(painter, option, index);
    style->drawPrimitive(element, &opt, painter);
}

QSize
RadioButtonItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle *style = QApplication::style();

    int radioWidth = style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
    int radioHeight = style->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
    int checkWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, &option);
    int checkHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, &option);

    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setWidth(qMax(size.width(), qMax(radioWidth, checkWidth)));
    size.setHeight(qMax(size.height(), qMax(radioHeight, checkHeight)));
    return size;
}

//
// Progress Bar
//
ProgressBarItemDelegate::ProgressBarItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void
ProgressBarItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    int data = index.data(Qt::UserRole).toInt();

    if (data != -2) {
        QStyleOptionProgressBar opt;
        opt.minimum = 0;
        opt.maximum = 100;
        opt.progress = data;
        opt.textVisible = false;
        opt.rect = option.rect;

        painter->save();
        painter->setClipRect(option.rect);
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, painter);
        painter->restore();
    }
}

//
// Svg Image Item
//
SvgImageItemDelegate::SvgImageItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void
SvgImageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QObject *obj = index.data(Qt::UserRole).value<QObject*>();
    QSvgRenderer *renderer = static_cast<QSvgRenderer*>(obj);

    if (renderer) {
        QRectF bounds(option.rect);
        bounds.translate(0, THUMB_SEP);
        bounds.setHeight(bounds.height() * 0.75 - THUMB_SEP);

        qreal w = bounds.width();
        qreal h = bounds.height();

        if (h > w) {
            bounds.translate(0, (h - w) / 2.0);
            bounds.setHeight(w);
        } else {
            bounds.translate((w - h) / 2.0, 0);
            bounds.setWidth(h);
        }

        painter->save();
        painter->setClipRect(bounds);
        renderer->render(painter, bounds);
        painter->restore();
    }
}

QSize
SvgImageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(1, option.fontMetrics.height() * 4);
}
