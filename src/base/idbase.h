// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "lib/uuid.h"

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE
class InfoWindow;
class TermFormat;

class IdBase: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ idStr)
    Q_PROPERTY(bool ours READ ours)

private:
    Tsq::Uuid m_id;
    QString m_idStr;

    InfoWindow *m_infoWindow = nullptr;

protected:
    AttributeMap m_attributes;
    bool m_ours;
    bool m_push;
    bool m_throttled = false;

    QString m_icons[2];
    int m_iconTypes[2];
    QString m_indicators[2];
    int m_indicatorTypes[2];

    TermFormat *m_format;

    void changeId(const Tsq::Uuid &id);

signals:
    void attributeChanged(const QString &key, const QString &value);
    void attributeRemoved(const QString &key);
    void iconsChanged(const QString *m_icons, const int *m_types);
    void indicatorsChanged();
    void throttleChanged(bool throttle);

public:
    IdBase(const Tsq::Uuid &id, bool ours, QObject *parent);

    inline const Tsq::Uuid& id() const { return m_id; }
    inline const QString& idStr() const { return m_idStr; }
    inline const auto& attributes() const { return m_attributes; }
    inline TermFormat* format() { return m_format; }

    inline const auto icons() const { return m_icons; }
    inline const auto iconTypes() const { return m_iconTypes; }
    inline const auto indicators() const { return m_indicators; }
    inline const auto indicatorTypes() const { return m_indicatorTypes; }
    inline const QString& iconStr() const { return m_icons[1]; }
    QString iconSpec() const;
    QIcon icon() const;

    QSvgRenderer* iconRenderer() const;
    QSvgRenderer* indicatorRenderer(int i) const;

    inline bool ours() const { return m_ours; }
    inline bool throttled() const { return m_throttled; }

    inline InfoWindow* infoWindow() { return m_infoWindow; }
    inline void setInfoWindow(InfoWindow *infoWindow) { m_infoWindow = infoWindow; }
    void setThrottled(bool throttled);
};
