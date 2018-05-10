// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/flags.h"

#include <QAbstractItemModel>
#include <QTreeView>
#include <QVector>

class IdBase;
class TermInstance;
class ServerInstance;
class InfoAnimation;

//
// Base Model
//
class InfoPropModel: public QAbstractItemModel
{
    Q_OBJECT

private:
    QVector<std::pair<QString,InfoAnimation*>> m_values;
    bool m_visible = false;

    QModelIndex indexForIndex(int idx, int col) const;
    virtual QString getRowName(int row) const = 0;

protected:
    unsigned m_nRows, m_nIndexes;
    const int8_t *m_counts;
    const int8_t *m_children;
    const int8_t *m_indexes;

    bool updateData(int idx, const QString &value);
    void updateList(int idx, QString value);
    void updateFlag(int idx, uint64_t flags, uint64_t flag);
    void updateChar(int idx, const char *chars, int charidx);

private slots:
    void handleAnimation(intptr_t idx);

public:
    InfoPropModel(unsigned nIndexes, IdBase *idbase, QWidget *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    inline void setVisible(bool visible) { m_visible = visible; }
};

//
// Terminal Model
//
class TermPropModel final: public InfoPropModel
{
    Q_OBJECT

private:
    TermInstance *m_term;

    virtual QString getRowName(int row) const;

private slots:
    void handleSizeChanged(QSize size);
    void handleBufferReset();
    void handleBufferChanged();
    void handleFetchChanged();

    void handleAttribute(const QString &key);
    void handleFlagsChanged(Tsq::TermFlags flags);
    void handleProcessChanged();
    void handleThrottleChanged(bool throttled);

public:
    TermPropModel(TermInstance *term, QWidget *parent);
};

//
// Server Model
//
class ServerPropModel final: public InfoPropModel
{
    Q_OBJECT

private:
    ServerInstance *m_server;

    virtual QString getRowName(int row) const;

private slots:
    void handleShortnameChanged(QString shortname);
    void handleThrottleChanged(bool throttled);

public:
    ServerPropModel(ServerInstance *server, QWidget *parent, bool setTitle);
};

//
// View
//
class InfoPropView final: public QTreeView
{
    Q_OBJECT

private:
    InfoPropModel *m_model;

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public:
    InfoPropView(InfoPropModel *model);
};
