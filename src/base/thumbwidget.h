// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPropertyAnimation;
QT_END_NAMESPACE
class TermInstance;
class TermManager;
class TermThumbport;
class TermBlinkTimer;
class TermEffectTimer;
class Region;

class ThumbWidget final: public QWidget, public DisplayIterator
{
    Q_OBJECT
    Q_PROPERTY(int resizeFade READ resizeFade WRITE setResizeFade)
    Q_PROPERTY(int flashFade READ flashFade WRITE setFlashFade)

private:
    TermManager *m_manager;
    TermThumbport *m_thumbport;

    bool m_visible = false;
    QColor m_bg, m_fg, m_flash;

    TermBlinkTimer *m_blink;
    TermEffectTimer *m_effects;
    int m_resizeFade = 255;
    int m_flashFade = 0;
    QPropertyAnimation *m_resizeAnim;
    QPropertyAnimation *m_flashAnim;

    QString m_resizeStr;
    QRect m_resizeRect;

    qreal m_thumbScale;

    qreal m_indexScale;
    QString m_indexStr;
    QRectF m_indexRect;

private:
    void updateIndexBounds();
    void updateEffects(const QSize &emulatorSize);
    void paintImage(QPainter &painter, const Region *region) const;

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void redisplay();
    void refocus();
    void recolor(QRgb bg, QRgb fg);
    void reindex();
    void refont(const QFont &font);
    void rescale();

    void blink();
    void bell();
    void startBlinking();

    void handleAlert();
    void handleSizeChanged(QSize emulatorSize);
    void handleShowIndexChanged(bool showIndex);
    void handleResizeEffectChanged(int resizeEffect);
    void handleProcessExited(QRgb exitColor);
    void handleCursorHighlight();

public:
    ThumbWidget(TermInstance *term, TermManager *manager, QWidget *parent);

    int heightForWidth(int w) const;
    int widthForHeight(int h) const;

    inline int resizeFade() const { return m_resizeFade; }
    void setResizeFade(int resizeFade);
    int flashFade() { return m_flashFade; }
    void setFlashFade(int flashFade);
};
