// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"

#include <QAbstractTableModel>
#include <QTableView>
#include <QMap>

class InfoAnimation;

//
// Model
//
class InfoAttrModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    struct Record {
        QString display;
        QString value;
        InfoAnimation *animation = nullptr;

        Record(const QString &value);
        void setValue(const QString &value);
    };

    QMap<QString,Record> m_map;
    bool m_visible = false;

private slots:
    void handleAnimation(intptr_t row);

public:
    InfoAttrModel(const AttributeMap &attributes, QWidget *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void data(int row, QString &name, QString &value) const;

    inline void setVisible(bool visible) { m_visible = visible; }

public slots:
    void handleAttributeChanged(const QString &key, const QString &value);
    void handleAttributeRemoved(const QString &key);
};

//
// View
//
class InfoAttrView final: public QTableView
{
    Q_OBJECT

public:
    InfoAttrView(InfoAttrModel *model);
};
