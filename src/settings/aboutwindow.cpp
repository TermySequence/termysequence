// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "aboutwindow.h"
#include "gitinfo.h"
#include "base/statuslabel.h"

#include <QFile>
#include <QLabel>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QKeyEvent>

#define TR_TAB1 TL("tab-title", "Information")
#define TR_TAB2 TL("tab-title", "Attributions")
#define TR_TEXT1 TL("window-text", "Version") + ':'
#define TR_TEXT2 TL("window-text", "Git tag") + ':'
#define TR_TEXT3 TL("window-text", "Git commit") + ':'

#ifdef NDEBUG
#define ABOUT_TRADENAME FRIENDLY_NAME "™"
#else
#define ABOUT_TRADENAME FRIENDLY_NAME
#endif

static const char *s_features[] = {
#if USE_FUSE
    "+fuse",
#endif
#if USE_SYSTEMD
    "+systemd",
#endif
    NULL
};

AboutWindow::AboutWindow()
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);

    QStringList features;
    for (const char **ptr = s_features; *ptr; ++ptr)
        features.append(*ptr);
    QString featureStr;
    if (!features.isEmpty())
        featureStr = A(" (") + features.join(' ') + ')';

    auto *mainLayout = new QVBoxLayout;

    auto *label = new QLabel(FRIENDLY_NAME " Qt Client");
    QFont font = label->font();
    font.setPointSize(font.pointSize() * 2);
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(label);

    auto *tabs = new QTabWidget;
    auto *layout = new QVBoxLayout;

    // Information tab
    auto *grid = new QGridLayout;
    grid->addWidget(new QLabel(TR_TEXT1), 0, 1);
    grid->addWidget(new QLabel(PROJECT_VERSION + featureStr), 0, 2);
    grid->addWidget(new QLabel(TR_TEXT2), 1, 1);
    grid->addWidget(new QLabel(GITDESC), 1, 2);
    grid->addWidget(new QLabel(TR_TEXT3), 2, 1);
    grid->addWidget(new QLabel(GITREV), 2, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(3, 1);
    layout->addLayout(grid);

    label = new StatusLabel(L("<a href='https://" PRODUCT_DOMAIN "'>https://" PRODUCT_DOMAIN "</a>"));
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    label = new QLabel(ABOUT_TRADENAME " Copyright © 2018 <a href='https://" ORG_DOMAIN "'>" ORG_NAME "</a>");
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    label = new QLabel("This is free software, made available under the terms of the "
                       "<a href='https://" PRODUCT_DOMAIN "/license/gpl-2.0.html'>GNU GPL, version 2</a>.");
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    label = new QLabel("This software comes with ABSOLUTELY NO WARRANTY, either expressed or implied.");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    auto *tab = new QWidget;
    tab->setLayout(layout);
    tabs->addTab(tab, TR_TAB1);

    // Attributions Tab
    auto *text = new QTextBrowser;
    text->setOpenExternalLinks(true);
    tabs->addTab(text, TR_TAB2);

    QFile file(":/dist/ATTRIBUTIONS.html");
    if (file.open(QFile::ReadOnly))
        text->setHtml(file.readAll());

    mainLayout->addWidget(tabs);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    show();
    raise();
    activateWindow();
}

bool
AboutWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    default:
        break;
    }

    return QWidget::event(event);
}
