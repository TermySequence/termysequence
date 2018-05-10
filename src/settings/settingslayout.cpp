// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/logging.h"
#include "base/statuslabel.h"
#include "settingslayout.h"
#include "settingwidget.h"
#include "base.h"
#include "global.h"

#include <QLabel>

SettingsLayout::SettingsLayout(SettingsBase *settings) :
    m_allCategories(TL("settings-category", "All"))
{
    setSizeConstraint(QLayout::SetMinimumSize);

    int row = 0, min1 = 0, min2 = 0, min3 = 0, w;
    QString headerStr("<u style='font-size:x-large'>%1</u>");

    for (const SettingDef *def = settings->defs(); def->factory; ++def, ++row) {
        QString descStr(QCoreApplication::translate("settings", def->description));
        QString categoryStr(QCoreApplication::translate("settings-category", def->category));
        QString keyStr = QString::fromLatin1(def->key);

        int category = m_allCategories.indexOf(categoryStr);
        if (category == -1) {
            category = m_allCategories.size();
            m_allCategories.append(categoryStr);

            GridItem header;
            header.label = new QLabel(headerStr.arg(categoryStr));
            header.label->setTextFormat(Qt::RichText);
            header.row = row;
            header.category = category;
            header.isHeader = true;
            header.isHidden = false;

            addWidget(header.label, row, 0, 1, 3);
            m_items.append(header);
            ++row;
        }

        GridItem item;
        item.label = new QLabel(descStr);
        item.widget = def->factory->createWidget(def, settings);
        item.link = makeLabel(settings->type(), keyStr);
        descStr += '\x1f' + keyStr.midRef(keyStr.indexOf('/') + 1);
        item.searchString = descStr.toLower();
        item.row = row;
        item.category = category;
        item.isHeader = false;
        item.isHidden = false;

        w = item.label->sizeHint().width();
        if (min1 < w)
            min1 = w;
        w = item.widget->sizeHint().width();
        if (min2 < w)
            min2 = w;
        if (min3 == 0)
            min3 = item.link->sizeHint().width();

        addWidget(item.label, row, 0);
        addWidget(item.widget, row, 1);
        addWidget(item.link, row, 2);
        m_items.append(item);
    }

    m_minimumWidth = min1 + min2 + min3 + horizontalSpacing();
    if (m_minimumWidth < 800)
        m_minimumWidth = 800;

    QWidget *filler = new QWidget();
    filler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(filler, row, 0, 1, 3);

    setColumnStretch(1, 1);
}

QLabel *
SettingsLayout::makeLabel(int type, const QString &keyStr)
{
    QString text = A("[<a href='%1'>?</a>]"), typeStr;

    switch (type) {
    case SettingsBase::Server:
        typeStr = A("server");
        break;
    case SettingsBase::Connect:
        typeStr = A("connection");
        break;
    case SettingsBase::Profile:
        typeStr = A("profile");
        break;
    case SettingsBase::Launch:
        typeStr = A("launcher");
        break;
    default:
        typeStr = A("global");
        break;
    }

    QString page = L("settings/%1.html#%2").arg(typeStr, keyStr);
    QString url = g_global->docUrl(page);

    StatusLabel *result = new StatusLabel(text.arg(url));
    result->setUrl(url);
    result->setToolTip(url);
    return result;
}

void
SettingsLayout::showItem(GridItem &item)
{
    item.isHidden = false;
    if (item.isHeader) {
        addWidget(item.label, item.row, 0, 1, 3);
        item.label->show();
    } else {
        addWidget(item.label, item.row, 0);
        addWidget(item.widget, item.row, 1);
        addWidget(item.link, item.row, 2);
        item.label->show();
        item.widget->show();
        item.link->show();
    }
}

void
SettingsLayout::hideItem(GridItem &item)
{
    item.isHidden = true;
    if (item.isHeader) {
        item.label->setVisible(false);
        removeWidget(item.label);
    } else {
        item.label->setVisible(false);
        item.widget->setVisible(false);
        item.link->setVisible(false);
        removeWidget(item.label);
        removeWidget(item.widget);
        removeWidget(item.link);
    }
}

void
SettingsLayout::setCategory(int index)
{
    m_searchCategory = index;
    runSearch();
}

void
SettingsLayout::setSearch(const QString &searchString)
{
    m_searchString = searchString.toLower();
    runSearch();
}

void
SettingsLayout::runSearch()
{
    for (GridItem &item: m_items) {
        bool hide = (m_searchCategory && m_searchCategory != item.category) ||
            (!m_searchString.isEmpty() && !item.isHeader &&
             !item.searchString.contains(m_searchString));

        if (!item.isHidden && hide)
            hideItem(item);
        else if (item.isHidden && !hide)
            showItem(item);
    }
}

void
SettingsLayout::resetSearch()
{
    for (GridItem &item: m_items)
        if (item.isHidden)
            showItem(item);
}

QSize
SettingsLayout::minimumSize() const
{
    // qCDebug(lcLayout) << "SettingsLayout minimumSize:" << QSize(m_minimumWidth, QGridLayout::minimumSize().height());
    return QSize(m_minimumWidth, QGridLayout::minimumSize().height());
}

QSize
SettingsLayout::sizeHint() const
{
    // qCDebug(lcLayout) << "SettingsLayout sizeHint:" << QSize(m_minimumWidth, QGridLayout::sizeHint().height());
    return QSize(m_minimumWidth, QGridLayout::sizeHint().height());
}
