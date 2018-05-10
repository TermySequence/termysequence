// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QGridLayout>
#include <QVector>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class SettingsBase;

struct GridItem {
    QLabel *label;
    QWidget *widget;
    QLabel *link;
    QString searchString;
    int row;
    short category;
    bool isHeader;
    bool isHidden;
};

class SettingsLayout final: public QGridLayout
{
private:
    int m_minimumWidth = 0;

    int m_searchCategory = 0;
    QString m_searchString;

    QStringList m_allCategories;
    QVector<GridItem> m_items;

    QLabel* makeLabel(int type, const QString &keyStr);

    void showItem(GridItem &item);
    void hideItem(GridItem &item);

    void runSearch();

public:
    SettingsLayout(SettingsBase *settings);

    void setCategory(int index);
    void setSearch(const QString &searchString);
    void resetSearch();

    const QStringList& categories() const { return m_allCategories; }
    QSize minimumSize() const;
    QSize sizeHint() const;
};
