// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/listener.h"
#include "imagewidget.h"
#include "imagedialog.h"
#include "settings.h"

#include <QLabel>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Choose") + A("...")
#define TR_BUTTON2 TL("input-button", "Clear")
#define TR_TEXT1 TL("window-text", "Avatar image is loaded in SVG format from:" \
    "\n%1\nSet or remove this file, then restart the application.")

//
// For normal icon categories
//
ImageWidget::ImageWidget(const SettingDef *def, SettingsBase *settings, int iconType) :
    SettingWidget(def, settings),
    m_iconType(iconType)
{
    auto *editButton = new IconButton(ICON_CHOOSE_ICON, TR_BUTTON1);
    auto *clearButton = new IconButton(ICON_CLEAR, TR_BUTTON2);
    m_pixmapHeight = editButton->sizeHint().height();

    m_image = new QLabel;
    m_text = new QLabel;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_image);
    layout->addWidget(m_text);
    layout->addWidget(editButton);
    layout->addWidget(clearButton);
    layout->addStretch(1);
    setLayout(layout);

    connect(editButton, SIGNAL(clicked()), SLOT(handleEdit()));
    connect(clearButton, SIGNAL(clicked()), SLOT(handleClear()));

    handleSettingChanged(m_value);
}

void
ImageWidget::handleEdit()
{
    auto *dialog = new ImageDialog(m_iconType, true, this);
    dialog->setCurrentIcon(m_value.toString());
    connect(dialog, &ImageDialog::okayed, [this](QString name) {
        setProperty(name);
    });
    dialog->show();
}

void
ImageWidget::handleClear()
{
    setProperty(g_mtstr);
}

void
ImageWidget::handleSettingChanged(const QVariant &value)
{
    QString icon = value.toString();
    m_text->setText(icon);

    if (icon.isEmpty())
        m_image->clear();
    else
        m_image->setPixmap(ThumbIcon::getPixmap((ThumbIcon::IconType)m_iconType,
                                                icon, m_pixmapHeight));
}

//
// For avatar icon
//
AvatarWidget::AvatarWidget()
{
    auto *button = new IconButton(ICON_CHOOSE_ICON, TR_BUTTON1);

    auto *image = new QLabel;
    image->setPixmap(ThumbIcon::getPixmap(ThumbIcon::AvatarType,
                                          g_listener->idStr(),
                                          button->sizeHint().height()));
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(image);
    layout->addWidget(button);
    layout->addStretch(1);
    setLayout(layout);

    connect(button, &QPushButton::clicked, this, [this]{
        infoBox(TR_TEXT1.arg(g_settings->avatarPath()), this)->show();
    });
}


ImageWidgetFactory::ImageWidgetFactory(int iconType) :
    m_iconType(iconType)
{
}

QWidget *
ImageWidgetFactory::createWidget(const SettingDef *def, SettingsBase *settings) const
{
    if (m_iconType == ThumbIcon::AvatarType)
        return new AvatarWidget;
    else
        return new ImageWidget(def, settings, m_iconType);
}
