// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/reaper.h"
#include "setupdialog.h"
#include "os/fd.h"
#include "os/process.h"
#include "config.h"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSocketNotifier>
#include <QFontDatabase>
#include <unistd.h>

#define TR_BUTTON1 TL("input-button", "Quit")
#define TR_BUTTON2 TL("input-button", "Skip")
#define TR_CHECK1 TL("input-checkbox", "Enable systemd user service")
#define TR_CHECK2 TL("input-checkbox", "Enable shell integration for bash")
#define TR_CHECK3 TL("input-checkbox", "Enable shell integration for zsh")
#define TR_TEXT1 TL("window-text", "Select setup tasks to perform") + ':'
#define TR_TEXT2 TL("window-text", "Access setup tasks at any time from the Help menu")
#define TR_TEXT3 TL("window-text", "Close all terminals and restart the application for changes to take effect")
#define TR_TITLE1 TL("window-title", "Setup Tasks")
#define TR_TITLE2 TL("window-title", "Setup Output")

//
// Output dialog
//
class SetupOutputDialog final: public QDialog
{
private:
    QPlainTextEdit *m_text;
    QPushButton *m_button;

    void appendLine(QString line);

public:
    SetupOutputDialog(bool initial, QWidget *parent);

    void appendOutput(QString output);
    void endOutput();

    QSize sizeHint() const { return QSize(640, 480); }
};

SetupOutputDialog::SetupOutputDialog(bool initial, QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(TR_TITLE2);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowModality(Qt::ApplicationModal);

    m_text = new QPlainTextEdit;
    m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_text->setReadOnly(true);

    auto *buttonBox = new QDialogButtonBox;
    m_button = buttonBox->addButton(QDialogButtonBox::Ok);
    if (initial) {
        auto *button = new IconButton(ICON_QUIT_APPLICATION, TR_BUTTON1);
        buttonBox->addButton(button, QDialogButtonBox::RejectRole);
        m_button->setEnabled(false);
    }

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_text, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

inline void
SetupOutputDialog::appendOutput(QString output)
{
    m_text->insertPlainText(output);
    m_text->ensureCursorVisible();
}

inline void
SetupOutputDialog::endOutput()
{
    m_button->setEnabled(true);
}

//
// Setup dialog
//
SetupDialog::SetupDialog(QWidget *parent) :
    QDialog(parent),
    m_initial(!parent)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    buttonBox->addHelpButton("setup-tasks");
    if (m_initial) {
        auto *button = new IconButton(ICON_QUIT_APPLICATION, TR_BUTTON1);
        buttonBox->addButton(button, QDialogButtonBox::RejectRole);
        button = new QPushButton(TR_BUTTON2);
        buttonBox->addButton(button, QDialogButtonBox::ActionRole);
        connect(button, SIGNAL(clicked()), SLOT(accept()));
    } else {
        buttonBox->addButton(QDialogButtonBox::Cancel);
    }

    auto *setupLayout = new QVBoxLayout;
    auto *setupGroup = new QGroupBox(TR_TEXT1);
    setupGroup->setLayout(setupLayout);

    const char *var;
    bool checked;

#if USE_SYSTEMD
    // systemd checkbox
    var = getenv("XDG_RUNTIME_DIR");
    checked = var && osFileExists(pr(QString(var) + A("/systemd")));
    setupLayout->addWidget(m_systemd = new QCheckBox(TR_CHECK1));
    m_systemd->setChecked(checked);
#endif
    // bash checkbox
    setupLayout->addWidget(m_bash = new QCheckBox(TR_CHECK2));
    m_bash->setChecked(true);

    // zsh checkbox
    var = getenv("HOME");
    checked = var && osFileExists(pr(QString(var) + A("/.zshrc")));
    setupLayout->addWidget(m_zsh = new QCheckBox(TR_CHECK3));
    m_zsh->setChecked(checked);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(setupGroup);
    mainLayout->addWidget(new QLabel(TR_TEXT2), 0, Qt::AlignCenter);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

SetupDialog::~SetupDialog()
{
    if (m_fd != -1)
        ::close(m_fd);
}

void
SetupDialog::handleAccept()
{
    QStringList command({ A(SETUP_NAME), A(SETUP_NAME) });

    if (USE_SYSTEMD && m_systemd->isChecked())
        command.append(A("--systemd"));
    if (m_bash->isChecked())
        command.append(A("--bash"));
    if (m_zsh->isChecked())
        command.append(A("--zsh"));

    if (command.size() > 2) {
        ForkParams params{};
        for (int i = 0; i < command.size(); ++i) {
            params.command.append(command.at(i).toStdString());
            if (i != command.size() - 1)
                params.command.push_back('\0');
        }
        params.dir = '/';

        try {
            int pid;
            m_fd = osForkProcess(params, &pid);
            ReaperThread::launchReaper(pid);

            m_dialog = new SetupOutputDialog(m_initial, this);
            connect(m_dialog, SIGNAL(accepted()), SLOT(accept()));
            connect(m_dialog, SIGNAL(rejected()), SLOT(reject()));
            m_dialog->show();

            m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
            connect(m_notifier, SIGNAL(activated(int)), SLOT(handleOutput(int)));
            return;
        } catch (const std::exception &) {
            // do nothing
        }
    }

    accept();
}

void
SetupDialog::handleOutput(int fd)
{
    char buf[READER_BUFSIZE];
    ssize_t rc = read(fd, buf, sizeof(buf));
    switch (rc) {
    default:
        m_dialog->appendOutput(QByteArray(buf, rc));
        break;
    case -1:
        if (errno == EINTR || errno == EAGAIN)
            break;
        // fallthru
    case 0:
        m_notifier->setEnabled(false);
        m_dialog->endOutput();
        if (!m_initial)
            m_dialog->appendOutput(TR_TEXT3);
    }
}
