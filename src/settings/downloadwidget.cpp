// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "downloadwidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QFileInfo>

DownloadWidget::DownloadWidget(const SettingDef *def, SettingsBase *settings, int type) :
    SettingWidget(def, settings)
{
    m_combo = new QComboBox();
    m_combo->installEventFilter(this);
    m_combo->setEditable(true);

    int sep = 1;
    QString path;

    if (type == 1) {
        path = TL("settings-enum", "Use global setting");
        m_combo->addItem(path, g_str_CURRENT_PROFILE);
        sep = 2;
    }

    path = TL("settings-enum", "Ask what to do");
    m_combo->addItem(path, g_str_PROMPT_PROFILE);
    m_combo->insertSeparator(sep);

    addPath(type, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    addPath(type, QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    addPath(type, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    addPath(type, A("/tmp"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_combo);
    setLayout(layout);

    handleSettingChanged(m_value);

    connect(m_combo,
            SIGNAL(currentTextChanged(const QString&)),
            SLOT(handleTextChanged(const QString&)));
}

void
DownloadWidget::addPath(int type, QString path)
{
    if (!path.isEmpty()) {
        switch (type) {
        case 2:
            path += A("/input.dat");
            break;
        case 3:
            path += A("/output.dat");
            break;
        }

        m_combo->addItem(path, path);
    }
}

void
DownloadWidget::handleTextChanged(const QString &text)
{
    QVariant value = text;

    for (int i = 0; i < m_combo->count(); ++i)
        if (m_combo->itemText(i) == value) {
            value = m_combo->itemData(i);
            break;
        }

    if (m_value != value)
        setProperty(value);
}

void
DownloadWidget::handleSettingChanged(const QVariant &value)
{
    for (int i = 0; i < m_combo->count(); ++i)
        if (m_combo->itemData(i) == value) {
            m_combo->setCurrentIndex(i);
            return;
        }

    QString text = value.toString();

    if (!text.isEmpty() && QFileInfo(text).isAbsolute())
        m_combo->setEditText(text);
    else
        m_combo->setCurrentIndex(0);
}

DownloadWidgetFactory::DownloadWidgetFactory(int type) :
    m_type(type)
{
}

QWidget *
DownloadWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    return new DownloadWidget(def, settings, m_type);
}
