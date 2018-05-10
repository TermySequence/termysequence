// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "themenew.h"
#include "theme.h"
#include "settings.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Low priority theme (listed at bottom)")
#define TR_FIELD1 TL("input-field", "Theme Name") + ':'
#define TR_FIELD2 TL("input-field", "Theme Group") + ':'
#define TR_TITLE1 TL("window-title", "New Theme")
#define TR_TITLE2 TL("window-title", "Rename Theme")

NewThemeDialog::NewThemeDialog(QWidget *parent, ThemeSettings *from):
    QDialog(parent),
    m_from(from)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(from ? TR_TITLE2 : TR_TITLE1);
    setWindowModality(Qt::WindowModal);

    if (from)
        from->takeReference();

    m_name = new QLineEdit;
    m_group = new QComboBox;
    m_lesser = new QCheckBox(TR_CHECK1);

    QStringList groups = g_settings->themeGroups();
    groups.sort();
    m_group->addItems(groups);
    m_group->setEditable(true);
    m_group->setCurrentIndex(0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_name);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addWidget(m_group);
    layout->addWidget(m_lesser);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SIGNAL(okayed()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(deleteLater()));
}

NewThemeDialog::~NewThemeDialog()
{
    if (m_from)
        m_from->putReference();
}

QString
NewThemeDialog::name() const
{
    return m_name->text();
}

void
NewThemeDialog::setName(const QString &name)
{
    m_name->setText(name);
}

QString
NewThemeDialog::group() const
{
    return m_group->currentText();
}

void
NewThemeDialog::setGroup(const QString &group)
{
    m_group->setCurrentText(group);
}

bool
NewThemeDialog::lesser() const
{
    return m_lesser->isChecked();
}

void
NewThemeDialog::setLesser(bool lesser)
{
    m_lesser->setChecked(lesser);
}
