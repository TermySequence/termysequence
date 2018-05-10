// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logging.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/conn.h"
#include "settings/state.h"
#include "settings/launcher.h"
#include "os/fd.h"
#include "lib/enums.h"

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Don't show this message again")
#define TR_TITLE1 TL("window-title", "Systemd Setup")
#define TR_TEXT1 TL("window-text", "Your persistent user server is not running as a systemd service.")
#define TR_TEXT2 TL("window-text", "This may cause it to be killed when you log out.")
#define TR_TEXT3 TL("window-text", "Run %1 now to set up user systemd services?")

class SystemdSetupDialog final: public QDialog
{
private:
    TermManager *m_manager;

    void handleAccept();

public:
    SystemdSetupDialog(TermManager *manager);
};

SystemdSetupDialog::SystemdSetupDialog(TermManager *manager) :
    QDialog(manager->parentWidget()),
    m_manager(manager)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QStringList parts;
    parts += TR_TEXT1;
    parts += TR_TEXT2 + '\n';
    parts += TR_TEXT3.arg(SYSTEMD_SETUP_NAME) + '\n';

    auto *checkBox = new QCheckBox(TR_CHECK1);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes|QDialogButtonBox::No);

    auto *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(parts.join('\n')));
    layout->addWidget(checkBox);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SystemdSetupDialog::handleAccept);
    connect(checkBox, &QCheckBox::toggled, g_state, &StateSettings::setSuppressSetup);
}

void
SystemdSetupDialog::handleAccept()
{
    QString cmd(SYSTEMD_SETUP_NAME " --server-pid '%1'||"
                "(echo Closing terminal in 30s;sleep 30)");

    auto *persistent = g_listener->persistentServer();
    auto *transient = g_listener->transientServer();

    if (persistent && transient) {
        LaunchSettings *launcher = new LaunchSettings;
        QString pid = persistent->attributes().value(g_attr_PID);
        QStringList command({"/bin/sh", "sh", "-c", cmd.arg(pid) });
        launcher->setCommand(command);
        m_manager->launchTerm(transient, launcher, AttributeMap());
        delete launcher;
    } else {
        qCCritical(lcCommand, "Cannot run setup: not connected");
    }

    accept();
}

bool
needSystemdSetup(ServerInstance *persistent, ServerInstance *transient)
{
    const char *var;

    return persistent && transient && !g_state->suppressSetup() &&
        persistent->attributes().value(g_attr_FLAVOR).toInt() != Tsq::FlavorActivated &&
        (var = getenv("XDG_RUNTIME_DIR")) &&
        osFileExists(pr(QString(var) + A("/systemd")));
}

void
showSystemdSetup(TermManager *manager)
{
    (new SystemdSetupDialog(manager))->show();
}
