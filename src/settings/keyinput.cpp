// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/keys.h"
#include "keyinput.h"

#include <QKeyEvent>
#include <QMouseEvent>

KeystrokeInput::KeystrokeInput()
{
    connect(this, &QLineEdit::textChanged, this, &KeystrokeInput::handleTextChanged);
    installEventFilter(this);
}

bool
KeystrokeInput::eventFilter(QObject *, QEvent *event)
{
    int key;

    switch (event->type()) {
    case QEvent::KeyPress:
        key = static_cast<QKeyEvent*>(event)->key();
        // Ignore pure modifier presses
        if (!isModifier(key))
            emit keystrokeReceived(key);
        return true;
    case QEvent::MouseButtonPress:
        key = static_cast<QMouseEvent*>(event)->button();
        // Ignore first three buttons
        if (key > Qt::MiddleButton)
            emit keystrokeReceived(key | KEY_BUTTON_BIT);
        return true;
    case QEvent::KeyRelease:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        return true;
    default:
        return false;
    }
}

void
KeystrokeInput::handleTextChanged(const QString &text)
{
    if (!text.isEmpty())
        clear();
}
