// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QPainter>
#include <QSizeF>

namespace drawing
{

extern void
paintU2500(QPainter &painter, const DisplayCell &i, QSizeF cellSize, QRgb bgval);

}
