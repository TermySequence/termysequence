// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "base/term.h"
#include "base/server.h"
#include "base/manager.h"
#include "base/mainwindow.h"
#include "base/pastetask.h"
#include "dropdialog.h"
#include "global.h"
#include "choicewidget.h"

#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#define TR_FIELD1 TL("input-field", "Select action to perform") + ':'
#define TR_TITLE1 TL("window-title", "Choose Drop Action")

DropDialog::DropDialog(TermInstance *term, ServerInstance *server,
                       int action, QList<QUrl> &urls, TermManager *manager) :
    QDialog(manager->parent()),
    m_term(term),
    m_server(server),
    m_manager(manager),
    m_action(action)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);

    m_urls.swap(urls);
}

void
DropDialog::start()
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    QLabel *header = new QLabel(TR_FIELD1);
    header->setAlignment(Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(header);

    const ChoiceDef *ptr = m_term ? g_dropConfig : g_serverDropConfig;
    bool first = true;
    for (++ptr; ptr->description; ++ptr) {
        QRadioButton *button = new QRadioButton(
            QCoreApplication::translate("settings-enum", ptr->description));

        if (first) {
            button->setChecked(true);
            first = false;
        }
        m_choices.insert(button, ptr->value.toInt());
        mainLayout->addWidget(button);
    }

    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    QObject *obj = m_term ? (QObject*)m_term : (QObject*)m_server;
    connect(obj, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
DropDialog::run()
{
    QString tmp;
    PasteFileTask *task = nullptr;

    switch (m_action) {
    case DropUploadCwd:
        tmp = m_server->cwd();
        if (!tmp.isEmpty())
            for (auto &url: qAsConst(m_urls))
                m_manager->actionUploadToDirectory(m_server->idStr(), tmp, url.toLocalFile());
        break;
    case DropUploadHome:
        tmp = m_server->attributes().value(g_attr_HOME);
        if (!tmp.isEmpty())
            for (auto &url: qAsConst(m_urls))
                m_manager->actionUploadToDirectory(m_server->idStr(), tmp, url.toLocalFile());
        break;
    case DropUploadTmp:
        for (auto &url: qAsConst(m_urls))
            m_manager->actionUploadToDirectory(m_server->idStr(), A("/tmp"), url.toLocalFile());
        break;
    case DropPasteName:
        for (auto &url: m_urls) {
            url.setHost(g_mtstr);
            tmp.append(TermUrl::quoted(url.toLocalFile()) + ' ');
        }
        m_term->pushInput(m_manager, tmp);
        break;
    case DropPasteContent:
        for (auto &url: m_urls) {
            url.setHost(g_mtstr);
            task = new PasteFileTask(m_term, url.toLocalFile(), task);
            task->start(m_manager);
        }
        break;
    default:
        break;
    }
}

void
DropDialog::handleAccept()
{
    m_action = DropNothing;

    for (auto i = m_choices.cbegin(), j = m_choices.cend(); i != j; ++i)
        if (i.key()->isChecked()) {
            m_action = *i;
            break;
        }

    run();
    accept();
}
