// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcCommand)
Q_DECLARE_LOGGING_CATEGORY(lcKeymap)
Q_DECLARE_LOGGING_CATEGORY(lcLayout)
Q_DECLARE_LOGGING_CATEGORY(lcSettings)
Q_DECLARE_LOGGING_CATEGORY(lcTerm)
Q_DECLARE_LOGGING_CATEGORY(lcPlugin)

#ifdef NDEBUG
#define logDebug(...)
#else
#include <QDebug>
#define logDebug qCDebug
#endif

#define OBJPROP_SDIR "serverRundir"
#define OBJPROP_ADIR "appRundir"
