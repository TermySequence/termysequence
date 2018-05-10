// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE
class SettingsBase;
class SettingsLayout;
class SettingsScroll;

class SettingsWindow final: public QWidget
{
    Q_OBJECT

private:
    SettingsBase *m_settings;

    QComboBox *m_categoryCombo;
    QPushButton *m_applyButton;
    QLineEdit *m_search;

    SettingsLayout *m_layout;
    SettingsScroll *m_scroll;

    bool m_accepting;

private slots:
    void handleChange();
    void handleLoad();
    void handleDefaults();

    void handleApply();
    void handleAccept();
    void handleRejected();

    void handleCategory(int index);
    void handleResetSearch();

protected:
    bool event(QEvent *event);

public:
    SettingsWindow(SettingsBase *settings);

    void bringUp();
};

extern SettingsWindow *g_globalwin;
