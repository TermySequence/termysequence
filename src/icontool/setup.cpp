// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "setup.h"
#include "settings.h"

#include <QLabel>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QTimer>

SetupDialog::SetupDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(A("Setup"));
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::NonModal);
    setSizeGripEnabled(true);

    m_repo = new QLineEdit(g_settings->repo());
    m_inst = new QLineEdit(g_settings->installdir());

    const auto &sources = g_settings->sources();
    m_sources = new QStandardItemModel(sources.size(), 3, this);
    m_sources->setHorizontalHeaderLabels(QStringList({ "Name", "Path", "SVG" }));

    int row = 0;
    for (const auto &i: sources) {
        auto *item = new QStandardItem(i.name);
        item->setEditable(false);
        m_sources->setItem(row, 0, item);

        item = new QStandardItem(i.path);
        item->setEditable(true);
        m_sources->setItem(row, 1, item);

        item = new QStandardItem(QString::number(i.needsvg));
        item->setEditable(true);
        m_sources->setItem(row, 2, item);
        ++row;
    }

    auto *view = new QTableView;
    view->setModel(m_sources);
    view->setShowGrid(false);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    view->verticalHeader()->hide();
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    view->horizontalHeader()->setStretchLastSection(true);
    view->resizeColumnsToContents();

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto *mainLayout = new QVBoxLayout;

    if (!parent) {
        auto *errorLayout = new QHBoxLayout;
        QStyle::StandardPixmap si = QStyle::SP_MessageBoxCritical;
        QIcon icon = style()->standardIcon(si, 0, this);
        int dim = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);

        if (!icon.isNull()) {
            QLabel *label = new QLabel;
            label->setPixmap(icon.pixmap(dim, dim));
            errorLayout->addWidget(label);
        }

        errorLayout->addWidget(new QLabel(A("Please enter correct setup information")), 1);
        mainLayout->addLayout(errorLayout);
    }

    mainLayout->addWidget(new QLabel(A("Repository root:")));
    mainLayout->addWidget(m_repo);
    mainLayout->addWidget(new QLabel(A("Icon sources:")));
    mainLayout->addWidget(view, 1);
    mainLayout->addWidget(new QLabel(A("Local install location:")));
    mainLayout->addWidget(m_inst);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
SetupDialog::handleAccept()
{
    QMap<QString,IconSource> sources;
    for (int i = 0, n = m_sources->rowCount(); i < n; ++i) {
        IconSource source = {
            m_sources->item(i)->text(),
            m_sources->item(i, 1)->text(),
            m_sources->item(i, 2)->text().toInt() != 0
        };
        sources.insert(source.name, source);
    }

    g_settings->setSources(std::move(sources));
    g_settings->setRepo(m_repo->text());
    g_settings->setInstallDir(m_inst->text());
    g_settings->saveSetup();
    accept();
}
