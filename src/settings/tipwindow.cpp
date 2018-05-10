// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/customaction.h"
#include "app/iconbutton.h"
#include "tipwindow.h"
#include "global.h"
#include "state.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/thumbicon.h"

#include <QPushButton>
#include <QCheckBox>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QDesktopServices>

#define TR_BUTTON1 TL("input-button", "Next Tip")
#define TR_FIELD1 TL("input-field", "Show on startup")
#define TR_TITLE1 TL("window-title", "Tip of the Day")

static const QRegularExpression s_linkre(L("<a (act|doc|href)=(?:'|\")(.*?)(?:'|\")>"),
                                         QRegularExpression::CaseInsensitiveOption);

TipsWindow *g_tipwin;

TipsWindow::TipsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_output = new QTextBrowser;
    m_output->setReadOnly(true);
    m_output->setOpenLinks(false);

    m_actimg = ThumbIcon::getThemePath(ICON_HELP_TOTD, A("16"));
    if (!m_actimg.isEmpty())
        m_actimg = L("<img src='%1' style='vertical-align:bottom'>").arg(m_actimg);
    m_docimg = ThumbIcon::getThemePath(ICON_HELP_CONTENTS, A("16"));
    if (!m_docimg.isEmpty())
        m_docimg = L("<img src='%1' style='vertical-align:bottom'>").arg(m_docimg);
    m_webimg = ThumbIcon::getThemePath(ICON_HELP_HOMEPAGE, A("16"));
    if (!m_webimg.isEmpty())
        m_webimg = L("<img src='%1' style='vertical-align:bottom'>").arg(m_webimg);

    auto *nextButton = new QPushButton(TR_BUTTON1);
    auto *checkBox = new QCheckBox(TR_FIELD1);
    checkBox->setChecked(g_state->showTotd());

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    buttonBox->addButton(nextButton, QDialogButtonBox::ActionRole);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(checkBox);
    buttonLayout->addWidget(buttonBox);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_output, 1);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(nextButton, SIGNAL(clicked()), SLOT(handleNextTip()));
    connect(checkBox, &QCheckBox::toggled, g_state, &StateSettings::setShowTotd);
    connect(m_output, &QTextBrowser::anchorClicked, this, &TipsWindow::handleLink);
}

bool
TipsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(TipsGeometryKey, saveGeometry());
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

void
TipsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(TipsGeometryKey));
    handleNextTip();

    show();
    raise();
    activateWindow();
}

void
TipsWindow::handleNextTip()
{
    auto spec = ActionFeature::getTip(g_listener->activeManager());
    QString tip = QCoreApplication::translate("totd", spec.at(0));

    for (int i = 1, n = spec.size(); i < n; ++i)
        tip = tip.arg(QString(spec[i]));

    QRegularExpressionMatch match;
    int offset = 0;
    while ((match = s_linkre.match(tip, offset)).hasMatch()) {
        QString link = match.captured(1).toLower();
        if (link == A("act")) {
            link = L("<a href='#@%1'>%2").arg(match.captured(2), m_actimg);
        } else if (link == A("doc")) {
            link = L("<a href='%1'>%2").arg(g_global->docUrl(match.captured(2)), m_docimg);
        } else {
            link = match.captured() + m_webimg;
        }
        offset = match.capturedStart();
        tip.replace(offset, match.capturedLength(), link);
        offset += link.size();
    }

    m_output->setHtml(tip);
}

void
TipsWindow::handleLink(const QUrl &url)
{
    QString urlStr = url.toString();

    if (urlStr.startsWith(A("#@"))) {
        urlStr = url.fragment(QUrl::FullyDecoded).mid(1);
        g_listener->activeManager()->invokeSlot(urlStr);
    } else {
        QDesktopServices::openUrl(url);
    }
}
