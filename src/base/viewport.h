// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "blinktimer.h"
#include "lib/types.h"

#include <QObject>
#include <QSize>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE
class TermInstance;
class TermBuffers;

class TermViewport: public QObject, public BlinkBase
{
    Q_OBJECT
    Q_PROPERTY(int focusEffect READ focusEffect WRITE setFocusEffect NOTIFY focusEffectChanged)
    Q_PROPERTY(int resizeEffect READ resizeEffect WRITE setResizeEffect NOTIFY resizeEffectChanged)
    Q_PROPERTY(int width READ width)
    Q_PROPERTY(int height READ height)
    Q_PROPERTY(bool primary READ primary)
    Q_PROPERTY(bool active READ focused)

    friend class DisplayIterator;

private:
    int m_focusEffect = 0;
    int m_resizeEffect = 0;

    QString toString() const;

protected:
    TermInstance *m_term;
    TermBuffers *m_buffers;

    QSize m_bounds;
    int m_offset = 0;

    bool m_primary = false;
    bool m_focused = false;

    void moveToEnd();
    void moveToOffset(int pos);
    void moveToRow(index_t *row, bool exact);

signals:
    void offsetChanged(int offset);
    void inputReceived();

    void focusEffectChanged(int focusEffect);
    void resizeEffectChanged(int resizeEffect);

private slots:
    virtual void updateRegions();

public:
    TermViewport(TermInstance *term, const QSize &size, QObject *parent);

    inline TermInstance* term() const { return m_term; }
    inline TermBuffers* buffers() const { return m_buffers; }
    inline int offset() const { return m_offset; }
    inline int height() const { return m_bounds.height(); }
    inline int width() const { return m_bounds.width(); }
    inline const QSize& size() const { return m_bounds; }
    inline bool primary() const { return m_primary; }
    inline bool focused() const { return m_focused; }

    virtual QPoint cursor() const = 0;
    virtual QPoint mousePos() const;

    void setPrimary(bool primary) { m_primary = primary; }
    void setFocused(bool focused) { m_focused = focused; }

    inline int focusEffect() { return m_focusEffect; }
    void setFocusEffect(int focusEffect);
    inline int resizeEffect() { return m_resizeEffect; }
    void setResizeEffect(int resizeEffect);

    int clipboardCopy() const;
    bool saveAs(QFile *file) const;
};
