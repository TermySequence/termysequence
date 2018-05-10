// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/color.h"

#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

//
// Animation
//
class InfoAnimation final: public QPropertyAnimation
{
    Q_OBJECT
    Q_PROPERTY(int fade READ fade WRITE setFade)

private:
    intptr_t m_data;
    QColor m_color;
    int m_fade = 0;

signals:
    void animationSignal(intptr_t data);

public:
    InfoAnimation(QObject *parent, intptr_t data = 0, int timescale = 1);

    inline intptr_t data() const { return m_data; }
    inline int fade() const { return m_fade; }
    inline void setData(intptr_t data) { m_data = data; }
    void setFade(int fade);

    void startColor(const QColor &color);
    void startColor(const QWidget *widget);
    void startColorName(ColorName name);
    void startVariant(const QVariant &color, const QWidget *widget);
    void stop();

    inline const QColor& color() const { return m_color; }
    inline QVariant colorVariant() const
    { return m_fade ? m_color : QVariant(); }

    QVariant blendedVariant(const QVariant &bg) const;
};

//
// Proxy
//
class ProxyAnimation final: public QObject
{
    Q_OBJECT

private:
    const intptr_t m_data;
    const InfoAnimation *m_source = nullptr;
    QMetaObject::Connection m_mocSource;

private slots:
    void handleAnimation();

signals:
    void animationSignal(intptr_t data);

public:
    ProxyAnimation(QObject *parent, intptr_t data);

    void setSource(const InfoAnimation *source);

    inline intptr_t data() const { return m_data; }
    QVariant colorVariant() const;
};
