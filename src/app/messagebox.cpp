// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "messagebox.h"

#include <QCoreApplication>

QMessageBox *
errBox(const QString &message, QWidget *parent)
{
    auto *box = new QMessageBox(QMessageBox::Critical,
                                QCoreApplication::translate("window-title", "Error"),
                                message, QMessageBox::Ok, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    return box;
}

QMessageBox *
errBox(const QString &title, const QString &message, QWidget *parent)
{
    auto *box = new QMessageBox(QMessageBox::Critical, title, message,
                                QMessageBox::Ok, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setTextFormat(Qt::RichText);
    return box;
}

QMessageBox *
infoBox(const QString &message, QWidget *parent)
{
    auto *box = new QMessageBox(QMessageBox::Information,
                                QCoreApplication::translate("window-title", "Information"),
                                message, QMessageBox::Ok, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    return box;
}

QMessageBox *
askBox(const QString &title, const QString &message, QWidget *parent)
{
    auto *box = new QMessageBox(QMessageBox::Question, title, message,
                                QMessageBox::Yes|QMessageBox::No, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setDefaultButton(QMessageBox::No);
    return box;
}

QColorDialog *
colorBox(QColor color, QWidget *parent)
{
    auto *box = new QColorDialog(color, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setOption(QColorDialog::DontUseNativeDialog, !NATIVE_DIALOGS);
    box->setWindowTitle(QCoreApplication::translate("window-title", "Select Color"));
    return box;
}

QFileDialog *
openBox(const QString &message, QWidget *parent)
{
    auto *box = new QFileDialog(parent, message);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setOption(QFileDialog::DontUseNativeDialog, !NATIVE_DIALOGS);
    box->setFileMode(QFileDialog::ExistingFile);
    return box;
}

QFileDialog *
saveBox(const QString &message, QWidget *parent)
{
    auto *box = new QFileDialog(parent, message);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setOption(QFileDialog::DontUseNativeDialog, !NATIVE_DIALOGS);
    box->setAcceptMode(QFileDialog::AcceptSave);
    box->setFileMode(QFileDialog::AnyFile);
    return box;
}

QInputDialog *
textBox(const QString &title, const QString &message,
        const QString &initial, QWidget *parent)
{
    auto *box = new QInputDialog(parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setWindowTitle(title);
    box->setLabelText(message);
    box->setInputMode(QInputDialog::TextInput);
    box->setTextValue(initial);
    return box;
}

QFontDialog *
fontBox(const QString &title, const QFont &initial,
        bool monospace, QWidget *parent)
{
    auto *box = new QFontDialog(initial, parent);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setOption(QFontDialog::DontUseNativeDialog, !NATIVE_DIALOGS);
    box->setOption(QFontDialog::MonospacedFonts, monospace);
    box->setWindowTitle(title);
    return box;
}
