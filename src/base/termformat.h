// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>
#include <QHash>

class TermInstance;
class ServerInstance;

class TermFormat final: public QObject
{
    Q_OBJECT

private:
    TermInstance *m_term;
    ServerInstance *m_server;

    QString m_captionFormat;
    QString m_tooltipFormat;
    QString m_titleFormat;
    QString m_badgeFormat;

    QString m_caption;
    QString m_tooltip;
    QString m_title;
    QString m_badge;

    int m_termCaptionLines = 0;

    QHash<QString,unsigned> m_active;

private:
    void removeActiveVariables(unsigned mask);
    void addActiveVariable(const QString &key, unsigned mask);
    void processFormat(const QString &format, unsigned mask);
    void checkStrings(unsigned mask);
    void buildString(const QString &format, QString &result);

signals:
    void captionChanged(const QString &value);
    void tooltipChanged(const QString &value);
    void titleChanged(const QString &value);
    void badgeChanged();

    void captionFormatChanged(const QString &format);
    void termCaptionLinesChanged(int lines);

private slots:
    void handleCaptionFormat(const QString &format);
    void handleTooltipFormat(const QString &format);
    void handleTitleFormat(const QString &format);

    void handleTermCaptionLines(const QString &format);

    void handleTermChange(const QString &key);
    void handleServerChange(const QString &key);

public:
    TermFormat(TermInstance *term);
    TermFormat(ServerInstance *server);

    inline const QString& caption() const { return m_caption; }
    inline const QString& tooltip() const { return m_tooltip; }
    inline const QString& title() const { return m_title; }
    inline const QString& badge() const { return m_badge; }

    inline const QString& captionFormat() const { return m_captionFormat; }
    inline int termCaptionLines() const { return m_termCaptionLines; }

    void reportBadgeFormat(const QString &format);
    void handleServerInfo();
};
