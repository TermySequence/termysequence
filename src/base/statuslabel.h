// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/flags.h"

#include <QLabel>

class TermKeymap;
class TermManager;
class InfoAnimation;
struct TermShortcut;

//
// Base class
//
class StatusLabel: public QLabel
{
    Q_OBJECT

private:
    QString m_url;

    bool m_hover = false;
    QColor m_bg;

protected:
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);

signals:
    void clicked();

public:
    StatusLabel();
    StatusLabel(const QString &text);

    void setUrl(const QString &url);
};

//
// Action class
//
class ActionLabel final: public StatusLabel
{
    Q_OBJECT

private:
    TermManager *m_manager;
    Tsq::TermFlags m_flags;
    QString m_slot, m_text;

    int m_fade = 0;
    InfoAnimation *m_animation = nullptr;

private slots:
    void handleLinkActivated();
    void handleAnimation();

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void invokingSlot(bool invoking);

public:
    ActionLabel(TermManager *manager, Tsq::TermFlags flags);
    ActionLabel(TermManager *manager, Tsq::TermFlags flags,
                const QString &slot, const QString &text);

    void setAction(const QString &slot, const QString &text);

    bool updateShortcut(const TermKeymap *keymap, TermShortcut *result);
    void setFlags(const TermKeymap *keymap, Tsq::TermFlags flags);

    void startAnimation();
};
