// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/thumbicon.h"
#include "settingwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

//
// For normal icon categories
//
class ImageWidget final: public SettingWidget
{
    Q_OBJECT

private:
    QLabel *m_image, *m_text;
    int m_iconType;
    int m_pixmapHeight;

private slots:
    void handleEdit();
    void handleClear();

protected:
    void handleSettingChanged(const QVariant &value);

public:
    ImageWidget(const SettingDef *def, SettingsBase *settings, int iconType);
};

//
// For avatar icon
//
class AvatarWidget final: public QWidget
{
public:
    AvatarWidget();
};


class ImageWidgetFactory final: public SettingWidgetFactory
{
private:
    int m_iconType;

public:
    ImageWidgetFactory(int iconType);

    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
