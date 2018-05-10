// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/format.h"
#include "termformat.h"
#include "term.h"
#include "server.h"
#include "settings/global.h"

#define MASK_CAPTION 1
#define MASK_TOOLTIP 2
#define MASK_TITLE   4
#define MASK_BADGE   8

TermFormat::TermFormat(TermInstance *term) :
    QObject(term),
    m_term(term),
    m_server(term->server())
{
    connect(m_server, SIGNAL(attributeChanged(QString,QString)), SLOT(handleServerChange(const QString&)));
    connect(m_server, SIGNAL(attributeRemoved(QString)), SLOT(handleServerChange(const QString&)));

    connect(m_term, SIGNAL(attributeChanged(QString,QString)), SLOT(handleTermChange(const QString&)));
    connect(m_term, SIGNAL(attributeRemoved(QString)), SLOT(handleTermChange(const QString&)));

    connect(g_global, SIGNAL(termCaptionChanged(QString)), SLOT(handleCaptionFormat(const QString&)));
    connect(g_global, SIGNAL(termTooltipChanged(QString)), SLOT(handleTooltipFormat(const QString&)));
    connect(g_global, SIGNAL(windowTitleChanged(QString)), SLOT(handleTitleFormat(const QString&)));

    handleCaptionFormat(g_global->termCaption());
    handleTooltipFormat(g_global->termTooltip());
    handleTitleFormat(g_global->windowTitle());
    reportBadgeFormat(m_term->badge());
}

TermFormat::TermFormat(ServerInstance *server) :
    QObject(server),
    m_term(nullptr),
    m_server(server)
{
    connect(m_server, SIGNAL(attributeChanged(QString,QString)), SLOT(handleServerChange(const QString&)));
    connect(m_server, SIGNAL(attributeRemoved(QString)), SLOT(handleServerChange(const QString&)));

    connect(g_global, SIGNAL(serverCaptionChanged(QString)), SLOT(handleCaptionFormat(const QString&)));
    connect(g_global, SIGNAL(serverTooltipChanged(QString)), SLOT(handleTooltipFormat(const QString&)));

    connect(g_global, SIGNAL(termCaptionChanged(QString)), SLOT(handleTermCaptionLines(const QString&)));

    handleCaptionFormat(g_global->serverCaption());
    handleTooltipFormat(g_global->serverTooltip());
    handleTermCaptionLines(g_global->termCaption());
}

inline void
TermFormat::removeActiveVariables(unsigned mask)
{
    for (auto i = m_active.begin(); i != m_active.end(); )
    {
        if (i.value() &= ~mask)
            ++i;
        else
            i = m_active.erase(i);
    }
}

inline void
TermFormat::addActiveVariable(const QString &key, unsigned mask)
{
    if (key.isEmpty())
        return;

    auto i = m_active.find(key);
    if (i != m_active.cend())
        *i |= mask;
    else
        m_active.insert(key, mask);
}

void
TermFormat::processFormat(const QString &format, unsigned mask)
{
    removeActiveVariables(mask);

    for (int i = 0; i < format.size() - 1; ++i) {
        if (format[i] != '\\' || format[++i] == '\\')
            continue;
        if (format[i] == '(') {
            int j = i + 1;

            while (++i < format.size()) {
                if (format[i] == ')') {
                    addActiveVariable(format.mid(j, i - j), mask);
                    break;
                }
            }
        }
    }
}

void
TermFormat::handleTermCaptionLines(const QString &format)
{
    // NOTE: This is used in server mode only for size calculations
    int lines = 1;

    for (int i = 0; i < format.size() - 1; ++i) {
        if (format[i] != '\\' || format[++i] == '\\')
            continue;
        if (format[i] == 'n')
            ++lines;
    }

    lines = format.size() ? lines : 0;

    if (m_termCaptionLines != lines) {
        m_termCaptionLines = lines;
        emit termCaptionLinesChanged(lines);
    }
}

void
TermFormat::buildString(const QString &format, QString &str)
{
    str = format;

    for (int i = 0; i < str.size() - 1; ++i) {
        if (str[i] != '\\')
            continue;

        int j = i++;

        if (str[i] == '\\') {
            str.replace(j, 2, '\\');
            --i;
        }
        else if (str[i] == 'n') {
            str.replace(j, 2, '\n');
            --i;
        }
        else if (str[i] == '(') {
            int k = i + 1;

            while (++i < str.size()) {
                if (str[i] == ')') {
                    QString val, var = str.mid(k, i - k);
                    if (var.isEmpty())
                        break;
                    else if (var.startsWith(g_attr_SERVER_PREFIX)) {
                        var.remove(0, sizeof(TSQ_ATTR_SERVER_PREFIX) - 1);
                        val = m_server->attributes().value(var);
                    }
                    else if (m_term)
                        val = m_term->attributes().value(var);

                    str.replace(j, i - j + 1, val);
                    i += val.size() + k - i - 3;
                    break;
                }
            }
        }
    }
}

void
TermFormat::handleCaptionFormat(const QString &format)
{
    processFormat(m_captionFormat = format, MASK_CAPTION);
    buildString(m_captionFormat, m_caption);
    emit captionChanged(m_caption);
    emit captionFormatChanged(m_captionFormat);
}

void
TermFormat::handleTooltipFormat(const QString &format)
{
    processFormat(m_tooltipFormat = format, MASK_TOOLTIP);
    buildString(m_tooltipFormat, m_tooltip);
    emit tooltipChanged(m_tooltip);
}

void
TermFormat::handleTitleFormat(const QString &format)
{
    processFormat(m_titleFormat = format, MASK_TITLE);
    buildString(m_titleFormat, m_title);
    emit titleChanged(m_title);
}

void
TermFormat::reportBadgeFormat(const QString &format)
{
    processFormat(m_badgeFormat = format, MASK_BADGE);
    buildString(m_badgeFormat, m_badge);
    emit badgeChanged();
}

void
TermFormat::checkStrings(unsigned active)
{
    if (active & MASK_CAPTION) {
        buildString(m_captionFormat, m_caption);
        emit captionChanged(m_caption);
    }
    if (active & MASK_TOOLTIP) {
        buildString(m_tooltipFormat, m_tooltip);
        emit tooltipChanged(m_tooltip);
    }
    if (active & MASK_TITLE) {
        buildString(m_titleFormat, m_title);
        emit titleChanged(m_title);
    }
    if (active & MASK_BADGE) {
        buildString(m_badgeFormat, m_badge);
        emit badgeChanged();
    }
}

void
TermFormat::handleTermChange(const QString &key)
{
    auto i = m_active.constFind(key);
    if (i != m_active.cend())
        checkStrings(*i);
}

void
TermFormat::handleServerChange(const QString &key)
{
    auto i = m_active.constFind(g_attr_SERVER_PREFIX + key);
    if (i != m_active.cend())
        checkStrings(*i);
}

void
TermFormat::handleServerInfo()
{
    unsigned active = m_active.value(g_attr_SERVER_PREFIX + g_attr_USER) |
        m_active.value(g_attr_SERVER_PREFIX + g_attr_NAME) |
        m_active.value(g_attr_SERVER_PREFIX + g_attr_HOST);

    checkStrings(active);
}
