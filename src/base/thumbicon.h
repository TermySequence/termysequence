// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE

#define TI_DEFAULT_NAME L("default")
#define TI_NONE_NAME L("none")

class ThumbIcon final: public QWidget
{
public:
    enum IconType {
        InvalidType = -1,
        TerminalType, ServerType, CommandType, IndicatorType, SemanticType,
        AvatarType, EmojiType, NIconType,
    };

private:
    QSvgRenderer *m_renderer;
    QSvgRenderer *m_renderers[2];

protected:
    void paintEvent(QPaintEvent *event);

public:
    ThumbIcon(IconType type, QWidget *parent);

    void setIcon(const QString &name, IconType type);
    void setIcons(const QString *names, const int *types);

    // global SVG icon cache methods
    typedef QVector<std::pair<QString,QIcon>> SIVector;

    static const QStringList& getAllNames(IconType type);
    static SIVector getAllIcons(IconType type);
    static const SIVector& getFavoriteIcons(IconType type);
    static void setFavoriteIcon(IconType type, const QString &name);

    static QSvgRenderer* getRenderer(IconType type, const QString &name);
    static QPixmap getPixmap(IconType type, const QString &name, int size);
    static QIcon getIcon(IconType type, const QString &name);
    static QString getPath(IconType type);

    static bool loadIcon(IconType type, const QString &name, const QByteArray &data);

    static QIcon fromTheme(const std::string &name);
    static QString getThemePath(const std::string &name, const QLatin1String &size);

    // post-global init
    static void initialize();
    static void teardown();
};

struct ThumbIconCache
{
    // pre-global init
    static void initialize();
    static QStringList allThemes;
};
