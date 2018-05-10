// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTextBrowser;
QT_END_NAMESPACE

class TipsWindow final: public QWidget
{
    Q_OBJECT

private:
    QTextBrowser *m_output;

    QString m_actimg, m_docimg, m_webimg;

private slots:
    void handleNextTip();
    void handleLink(const QUrl &url);

protected:
    bool event(QEvent *event);

public:
    TipsWindow();

    QSize sizeHint() const { return QSize(640, 320); }

    void bringUp();
};

extern TipsWindow *g_tipwin;
