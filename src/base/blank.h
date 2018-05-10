// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QScrollBar;
QT_END_NAMESPACE
class TermManager;
class ProfileSettings;

class TermBlank final: public QWidget
{
    Q_OBJECT

private:
    TermManager *m_manager;

    QLabel *m_label;
    QScrollBar *m_scroll;

    QSize m_currentSize;
    QSize m_sizeHint;

    bool m_focused;

    void calculateSizeHint(const ProfileSettings *profile);
    void refocus();

protected:
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

public:
    TermBlank(TermManager *manager, QWidget *parent = 0);

    inline void setSize(const QSize &size) { m_currentSize = size; }

    QSize calculateTermSize(const ProfileSettings *profile) const;
    QSize sizeHint() const;
};
