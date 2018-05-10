// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QSettings>
#include <QHash>

class SettingWidgetFactory;
class InfoAnimation;

struct SettingDef
{
    const char *key;
    const char *property;
    QVariant::Type type;
    const char *category;
    const char *description;
    const SettingWidgetFactory *factory;
};

class SettingsBase: public QSettings
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)

public:
    enum SettingsType { Global, Profile, Launch, Connect, Server, Alert };

    struct SettingsDef {
        const SettingsType type;
        const SettingDef *defs;
        QHash<QByteArray,QVariant> defaults;
    };

private:
    const struct SettingsDef &m_def;
    bool m_loaded = false;

    void writeSetting(const SettingDef *s, const QVariant &value);

protected:
    bool m_writable;
    bool m_reserved = false;
    bool m_default = false;
    unsigned m_favorite = 0;

    QString m_name;

    int m_refcount = 1;
    int m_row = -1;

private:
    InfoAnimation *m_animation = nullptr;

signals:
    void settingChanged(const char *property, QVariant value);
    void settingsLoaded();

public:
    SettingsBase(const SettingsDef &def,
                 const QString &path = QString(), bool writable = true);

    inline SettingsType type() const { return m_def.type; }
    inline bool reserved() const { return m_reserved; }
    inline bool isdefault() const { return m_default; }
    inline bool isfavorite() const { return m_favorite; }
    inline unsigned favorite() const { return m_favorite; }
    inline const QString& name() const { return m_name; }
    inline const SettingDef *defs() const { return m_def.defs; }
    inline int row() const { return m_row; }
    inline InfoAnimation* animation() const { return m_animation; }

    const QVariant defaultValue(const SettingDef *def) const;
    const SettingDef* getDef(const char *key) const;

    void takeReference();
    void putReference();
    void putReferenceAndDeanimate();

    inline void setRow(int row) { m_row = row; }
    void adjustRow(int delta);
    InfoAnimation* createAnimation(int timescale = 1);

public slots:
    virtual void loadSettings();
    void saveSettings();
    void saveSetting(const char *property, const QVariant &value);
    void copySettings(SettingsBase *other) const;
    void resetToDefaults();

    void activate();
};

inline void
SettingsBase::activate()
{
    if (!m_loaded)
        loadSettings();
}

#define VALPROP(type, get, set) \
    private: type m_ ## get; \
    public: inline type get() const { return m_ ## get; } \
    void set(type get);

#define REFPROP(type, get, set) \
    private: type m_ ## get; \
    public: inline const type & get() const { return m_ ## get; } \
    void set(const type & get);
