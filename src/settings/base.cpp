// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/logging.h"
#include "base.h"
#include "base/infoanim.h"

#include <cassert>

SettingsBase::SettingsBase(const SettingsDef &def,
                           const QString &path, bool writable) :
    QSettings(path.isEmpty() ? A("/dev/null") : path, QSettings::IniFormat),
    m_def(def),
    m_writable(!path.isEmpty() && writable)
{
}

void
SettingsBase::takeReference()
{
    ++m_refcount;
}

void
SettingsBase::putReference()
{
    if (--m_refcount == 0)
        delete this;
}

void
SettingsBase::putReferenceAndDeanimate()
{
    if (--m_refcount) {
        if (m_animation) {
            delete m_animation;
            m_animation = nullptr;
        }
        m_row = -1;
        m_writable = false;
    } else {
        delete this;
    }
}

void
SettingsBase::adjustRow(int delta)
{
    m_row += delta;

    if (m_animation)
        m_animation->setData(m_row);
}

InfoAnimation *
SettingsBase::createAnimation(int timescale)
{
    return m_animation = new InfoAnimation(this, m_row, timescale);
}

const QVariant
SettingsBase::defaultValue(const SettingDef *s) const
{
    return m_def.defaults.value(s->property, QVariant(s->type));
}

const SettingDef *
SettingsBase::getDef(const char *key) const
{
    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (!strcmp(s->key, key))
            return s;

    return nullptr;
}

void
SettingsBase::loadSettings()
{
    m_loaded = true;

    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (s->property) {
            setProperty(s->property, value(s->key, defaultValue(s)));
            qCDebug(lcSettings) << metaObject()->className() << "loaded" << s->property
                                << "with value:" << property(s->property);
        }

    emit settingsLoaded();
}

inline void
SettingsBase::writeSetting(const SettingDef *s, const QVariant &value)
{
    switch (s->type) {
    case QVariant::String:
    case QVariant::StringList:
        if (value == defaultValue(s)) {
            remove(s->key);
            break;
        }
        // fallthru
    default:
        setValue(s->key, value);
    }
}

void
SettingsBase::saveSetting(const char *property, const QVariant &value)
{
    assert(m_loaded);

    if (!m_writable)
        return;

    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (s->property && !strcmp(s->property, property))
            writeSetting(s, value);

    sync();
}

void
SettingsBase::saveSettings()
{
    assert(m_loaded);

    if (!m_writable)
        goto out;

    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (s->property)
            writeSetting(s, property(s->property));

    sync();
out:
    emit settingsLoaded();
}

void
SettingsBase::copySettings(SettingsBase *other) const
{
    assert(m_loaded);
    other->m_loaded = true;

    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (s->property)
            other->setProperty(s->property, property(s->property));
}

void
SettingsBase::resetToDefaults()
{
    assert(m_loaded);

    for (const SettingDef *s = m_def.defs; s->key; ++s)
        if (s->property)
            setProperty(s->property, defaultValue(s));
}
