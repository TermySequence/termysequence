// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseinline.h"

#include <QPixmap>

QT_BEGIN_NAMESPACE
class QMovie;
QT_END_NAMESPACE
struct TermContent;

class InlineImage final: public InlineBase
{
    Q_OBJECT

private:
    QPixmap m_pixmap;
    TermContent *m_content = nullptr;
    QMovie *m_movie = nullptr;

    QString m_contentId;
    QString m_icon;
    bool m_inline;
    bool m_keepAspect;
    bool m_needContent;

    QMenu* getPopup(MainWindow *window);
    void getDragData(QMimeData *data, QDrag *drag);
    void open();
    void clipboardCopy();
    void textSelect();
    void textWrite();

    void stopMovie();
    void setContent();

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void handleContentRemoved(TermContent *content);
    void handleContentFetching(TermContent *content);
    void handleMovieUpdate();

public:
    InlineImage(TermWidget *parent);
    ~InlineImage();

    void setRegion(const Region *region);
    void bringDown();
    void bringUp();
};
