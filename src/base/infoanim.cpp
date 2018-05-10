// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "infoanim.h"
#include "settings/global.h"

#include <QWidget>

//
// Animation
//
InfoAnimation::InfoAnimation(QObject *parent, intptr_t data, int timescale) :
    QPropertyAnimation(parent),
    m_data(data)
{
    setTargetObject(this);
    setPropertyName(QByteArrayLiteral("fade"));

    setEasingCurve(QEasingCurve::OutCubic);
    setDuration(BLINK_TIME * timescale);
    setStartValue(200);
    setEndValue(0);
}

void
InfoAnimation::setFade(int fade)
{
    m_color.setAlpha(m_fade = fade);
    emit animationSignal(m_data);
}

void
InfoAnimation::startColor(const QColor &color)
{
    if (state() != QAbstractAnimation::Stopped)
        QPropertyAnimation::stop();

    m_color = color;
    start();
}

void
InfoAnimation::startColor(const QWidget *widget)
{
    startColor(widget->palette().color(widget->foregroundRole()));
}

void
InfoAnimation::startColorName(ColorName name)
{
    startColor(g_global->color(name));
}

void
InfoAnimation::startVariant(const QVariant &color, const QWidget *widget)
{
    startColor(color.isValid() ?
               color.value<QColor>() :
               widget->palette().color(widget->foregroundRole()));
}

QVariant
InfoAnimation::blendedVariant(const QVariant &bg) const
{
    if (!bg.isValid())
        return colorVariant();

    QColor c = bg.value<QColor>();
    return m_fade ?
        QColor(Colors::blend0a(c.rgb(), m_color.rgb(), m_fade)) :
        c;
}

void
InfoAnimation::stop()
{
    QPropertyAnimation::stop();
    m_fade = 0;
}

//
// Proxy
//
ProxyAnimation::ProxyAnimation(QObject *parent, intptr_t data) :
    QObject(parent),
    m_data(data)
{}

void
ProxyAnimation::setSource(const InfoAnimation *source)
{
    if (m_source)
        disconnect(m_mocSource);

    m_source = source;
    m_mocSource = connect(source, SIGNAL(animationSignal(intptr_t)),
                          SLOT(handleAnimation()));
}

void
ProxyAnimation::handleAnimation()
{
    emit animationSignal(m_data);
}

QVariant
ProxyAnimation::colorVariant() const
{
    return m_source ? m_source->colorVariant() : QVariant();
}
