// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QHash>
#include <QCoreApplication>
#include <QMargins>

typedef QHash<QString,QString> AttributeMap;

// The empty string
extern const QString g_mtstr;

// Macros for string literals
#define A(x) QLatin1String(x)
#define L(x) QStringLiteral(x)
#define B(x) QByteArrayLiteral(x)
#define pr(x) ((x).toUtf8().constData())

#define TN(c, x) QT_TRANSLATE_NOOP(c, x)
#define T3(c, x, d) QT_TRANSLATE_NOOP3(c, x, d)
#define TL(c, x, ...) QCoreApplication::translate(c, x, ##__VA_ARGS__)

#define C(x) QChar(x)
#define Ch0 QChar('0')

// The empty margins
extern const QMargins g_mtmargins;
