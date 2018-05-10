// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QAbstractTableModel>
#include <QTableView>

//
// Model
//
class ImageModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    QStringList m_all, m_cur;
    int m_type;

public:
    ImageModel(int iconType, QObject *parent);

    void setSearchString(const QString &text);
    int indexOf(const QString &name);

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

//
// View
//
class ImageView final: public QTableView
{
    Q_OBJECT

private:
    ImageModel *m_model;

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void doubleClicked();

public:
    ImageView(ImageModel *model);

    QString selectedImage() const;
    void clearSelectedImage();
    void setSelectedImage(const QString &cur);
};
