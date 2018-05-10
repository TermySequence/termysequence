// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "port.h"

#include <QAbstractTableModel>
#include <QTableView>

//
// Model
//
class PortEditModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    PortFwdList m_ports;
    QStringList m_portsList;

signals:
    void portsChanged();

public:
    PortEditModel(QObject *parent);

    inline const QStringList& portsList() const { return m_portsList; }
    void setPortsList(const QStringList &portsList);

    inline const PortFwdRule& rule(int row) const { return m_ports.at(row); }
    inline bool containsRule(const PortFwdRule &rule) const { return m_ports.contains(rule); }
    int addRule(const PortFwdRule &rule);
    void removeRule(int row);
    void setRule(int row, const PortFwdRule &rule);

    void toggleAuto(const QModelIndex &index);
    int autoCount() const;

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

//
// View
//
class PortEditView final: public QTableView
{
private:
    PortEditModel *m_model;

    void handleClicked(const QModelIndex &index);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    PortEditView(PortEditModel *model);
};
