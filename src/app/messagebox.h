// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>

extern QMessageBox *
errBox(const QString &message, QWidget *parent);

extern QMessageBox *
errBox(const QString &title, const QString &message, QWidget *parent);

extern QMessageBox *
infoBox(const QString &message, QWidget *parent);

extern QMessageBox *
askBox(const QString &title, const QString &message, QWidget *parent);

extern QColorDialog *
colorBox(QColor color, QWidget *parent);

extern QFileDialog *
openBox(const QString &message, QWidget *parent);

extern QFileDialog *
saveBox(const QString &message, QWidget *parent);

extern QInputDialog *
textBox(const QString &title, const QString &message,
        const QString &initial, QWidget *parent);

extern QFontDialog *
fontBox(const QString &title, const QFont &initial,
        bool monospace, QWidget *parent);
