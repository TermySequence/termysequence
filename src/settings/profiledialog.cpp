// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/customaction.h"
#include "app/iconbutton.h"
#include "app/slotcombo.h"
#include "base/idbase.h"
#include "base/listener.h"
#include "base/manager.h"
#include "profiledialog.h"
#include "settings.h"
#include "state.h"

#include <QLabel>
#include <QVBoxLayout>

#define TR_BUTTON1 TL("input-button", "Manage") + A("...")
#define TR_BUTTON2 TL("input-button", "Clear History")
#define TR_TITLE1 TL("window-title", "Select Profile")
#define TR_TITLE2 TL("window-title", "Select Launcher")
#define TR_TITLE3 TL("window-title", "Select Alert")
#define TR_TITLE4 TL("window-title", "Invoke Action")

//
// Choose profile
//
ProfileDialog::ProfileDialog(IdBase *idbase, QWidget *parent) :
    QDialog(parent),
    m_idStr(idbase->idStr())
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);

    m_combo = new QComboBox;
    for (auto &i: g_settings->allProfiles())
        m_combo->addItem(i.second, i.first);

    auto *button = new QPushButton(TR_BUTTON1);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttonBox->addButton(button, QDialogButtonBox::ActionRole);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(windowTitle() + ':'));
    layout->addWidget(m_combo);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(idbase, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccepted()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(button, SIGNAL(clicked()), SLOT(handleManage()));
}

void
ProfileDialog::handleAccepted()
{
    emit okayed(m_combo->currentText(), m_idStr);
    accept();
}

void
ProfileDialog::handleManage()
{
    auto *manager = g_listener->activeManager();
    if (manager) {
        manager->actionManageProfiles();
        accept();
    }
}

//
// Choose launcher
//
LauncherDialog::LauncherDialog(QWidget *parent, IdBase *idbase, const QString &uri,
                               const QString &subs) :
    QDialog(parent),
    m_idStr(idbase->idStr()),
    m_uri(uri),
    m_subs(subs)
{
    setWindowTitle(TR_TITLE2);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);

    m_combo = new QComboBox;
    for (auto &i: g_settings->allLaunchers())
        m_combo->addItem(i.second, i.first);

    auto *button = new QPushButton(TR_BUTTON1);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttonBox->addButton(button, QDialogButtonBox::ActionRole);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(windowTitle() + ':'));
    layout->addWidget(m_combo);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(idbase, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccepted()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(button, SIGNAL(clicked()), SLOT(handleManage()));
}

void
LauncherDialog::handleAccepted()
{
    emit okayed(m_combo->currentText(), m_idStr, m_uri, m_subs);
    accept();
}

void
LauncherDialog::handleManage()
{
    auto *manager = g_listener->activeManager();
    if (manager) {
        manager->actionManageLaunchers();
        accept();
    }
}

//
// Choose alert
//
AlertDialog::AlertDialog(IdBase *idbase, QWidget *parent) :
    QDialog(parent),
    m_idStr(idbase->idStr())
{
    setWindowTitle(TR_TITLE3);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);

    m_combo = new QComboBox;
    for (auto &i: g_settings->allAlerts())
        m_combo->addItem(i.second, i.first);

    auto *button = new QPushButton(TR_BUTTON1);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttonBox->addButton(button, QDialogButtonBox::ActionRole);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(windowTitle() + ':'));
    layout->addWidget(m_combo);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(idbase, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccepted()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(button, SIGNAL(clicked()), SLOT(handleManage()));
}

void
AlertDialog::handleAccepted()
{
    emit okayed(m_combo->currentText(), m_idStr);
    accept();
}

void
AlertDialog::handleManage()
{
    auto *manager = g_listener->activeManager();
    if (manager) {
        manager->actionManageAlerts();
        accept();
    }
}

//
// Choose slot
//
static void
populateSlots(QComboBox *combo, const QStringList &history)
{
    QStringList items = ActionFeature::customSlots();
    items.append(TermManager::allSlots());

    for (const QString &str: history)
        items.removeOne(str);

    combo->addItems(items);
    combo->insertSeparator(0);
    combo->insertItems(0, history);
    combo->setCurrentIndex(0);
}

SlotDialog::SlotDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(TR_TITLE4);
    setAttribute(Qt::WA_DeleteOnClose, true);

    m_combo = new SlotCombo;

    m_history = g_state->actionHistory();
    populateSlots(m_combo, m_history);

    auto *button = new IconButton(ICON_CLEAN, TR_BUTTON2);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttonBox->addButton(button, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("/actions");

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(windowTitle() + ':'));
    layout->addWidget(m_combo);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccepted()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(button, SIGNAL(clicked()), SLOT(handleClear()));
}

void
SlotDialog::handleAccepted()
{
    QString slot = m_combo->currentText();
    if (m_history.value(0) != slot) {
        m_history.removeAll(slot);
        m_history.prepend(slot);
        g_state->setActionHistory(m_history);
    }
    emit okayed(slot, false);
    accept();
}

void
SlotDialog::handleClear()
{
    m_history.clear();
    g_state->setActionHistory(m_history);

    m_combo->clear();
    populateSlots(m_combo, m_history);
}

QSize
SlotDialog::sizeHint() const
{
    QSize size = QDialog::sizeHint();
    if (size.width() < 512)
        size.setWidth(512);
    return size;
}
