// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/logging.h"
#include "settingswindow.h"
#include "settingslayout.h"
#include "base.h"
#include "global.h"
#include "state.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QKeyEvent>

#define TR_BUTTON1 TL("input-button", "Clear Search")
#define TR_FIELD1 TL("input-field", "Category") + ':'
#define TR_FIELD2 TL("input-field", "Search") + ':'

SettingsWindow *g_globalwin;

//
// Scroll
//
class SettingsScroll final: public QScrollArea
{
private:
    QLayout *m_layout;

public:
    SettingsScroll(QLayout *layout);

    QSize minimumSize() const;
    QSize sizeHint() const;
};

SettingsScroll::SettingsScroll(QLayout *layout): m_layout(layout)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    QWidget *scrollWidget = new QWidget;
    scrollWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    scrollWidget->setLayout(layout);
    setWidget(scrollWidget);
}

QSize
SettingsScroll::minimumSize() const
{
    int overhead = 2 * frameWidth() + verticalScrollBar()->minimumSize().width();
    QSize result = QSize(m_layout->minimumSize().width() + overhead, QScrollArea::minimumSize().height());
    // qCDebug(lcLayout) << "SettingsScroll minimumSize:" << result;
    return result;
}

QSize
SettingsScroll::sizeHint() const
{
    int overhead = 2 * frameWidth() + verticalScrollBar()->sizeHint().width();
    QSize result = QSize(m_layout->sizeHint().width() + overhead, 600);
    // qCDebug(lcLayout) << "SettingsScroll sizeHint:" << result;
    return result;
}

//
// Window
//
static std::pair<StateSettingsKey,StateSettingsKey>
getSettingsKeys(SettingsBase::SettingsType type)
{
    switch (type) {
    case SettingsBase::Profile:
        return std::make_pair(ProfileGeometryKey, ProfileCategoryKey);
    case SettingsBase::Launch:
        return std::make_pair(LauncherGeometryKey, LauncherCategoryKey);
    case SettingsBase::Connect:
        return std::make_pair(ConnectGeometryKey, ConnectCategoryKey);
    case SettingsBase::Server:
        return std::make_pair(ServerGeometryKey, ServerCategoryKey);
    case SettingsBase::Alert:
        return std::make_pair(AlertGeometryKey, AlertCategoryKey);
    default:
        return std::make_pair(GlobalGeometryKey, GlobalCategoryKey);
    }
}

SettingsWindow::SettingsWindow(SettingsBase *settings): m_settings(settings)
{
    setAttribute(Qt::WA_QuitOnClose, false);

    settings->activate();
    m_layout = new SettingsLayout(settings);
    m_scroll = new SettingsScroll(m_layout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|
        QDialogButtonBox::Apply|QDialogButtonBox::RestoreDefaults);

    buttonBox->addHelpButton("settings-editor");
    QPushButton *defaultsButton = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setEnabled(false);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItems(m_layout->categories());
    m_categoryCombo->setCurrentIndex(0);
    m_search = new QLineEdit;
    QPushButton *searchButton = new IconButton(ICON_CLEAR, TR_BUTTON1);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel(TR_FIELD1));
    searchLayout->addWidget(m_categoryCombo, 1);
    searchLayout->addWidget(new QLabel(TR_FIELD2));
    searchLayout->addWidget(m_search, 2);
    searchLayout->addWidget(searchButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(searchLayout);
    layout->addWidget(m_scroll, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(settings, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_applyButton, SIGNAL(clicked()), SLOT(handleApply()));
    connect(defaultsButton, SIGNAL(clicked()), SLOT(handleDefaults()));
    connect(settings, SIGNAL(settingChanged(const char *,QVariant)), SLOT(handleChange()));
    connect(settings, SIGNAL(settingsLoaded()), SLOT(handleLoad()));

    connect(m_categoryCombo, SIGNAL(currentIndexChanged(int)), SLOT(handleCategory(int)));
    connect(m_search, &QLineEdit::textChanged, m_layout, &SettingsLayout::setSearch);
    connect(searchButton, SIGNAL(clicked()), SLOT(handleResetSearch()));

    auto keys = getSettingsKeys(settings->type());
    m_categoryCombo->setCurrentIndex(g_state->fetch(keys.second).toInt());
}

void
SettingsWindow::handleCategory(int index)
{
    m_layout->setCategory(index);
    m_scroll->verticalScrollBar()->setValue(0);
}

void
SettingsWindow::handleResetSearch()
{
    m_categoryCombo->setCurrentIndex(0);
    m_search->clear();
    m_layout->resetSearch();
}

bool
SettingsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    default:
        break;
    case QEvent::Close:
        if (!m_accepting)
            handleRejected();

        auto keys = getSettingsKeys(m_settings->type());
        g_state->store(keys.first, saveGeometry());
        g_state->store(keys.second, QByteArray::number(m_categoryCombo->currentIndex()));
        break;
    }

    return QWidget::event(event);
}

void
SettingsWindow::bringUp()
{
    auto keys = getSettingsKeys(m_settings->type());
    restoreGeometry(g_state->fetch(keys.first));

    m_accepting = false;

    show();
    raise();
    activateWindow();
}

void
SettingsWindow::handleChange()
{
    m_applyButton->setEnabled(true);
}

void
SettingsWindow::handleLoad()
{
    m_applyButton->setEnabled(false);
}

void
SettingsWindow::handleDefaults()
{
    m_settings->resetToDefaults();
}

void
SettingsWindow::handleApply()
{
    m_settings->saveSettings();
}

void
SettingsWindow::handleAccept()
{
    handleApply();
    m_accepting = true;
    close();
}

void
SettingsWindow::handleRejected()
{
    if (m_applyButton->isEnabled())
        m_settings->loadSettings();
}
