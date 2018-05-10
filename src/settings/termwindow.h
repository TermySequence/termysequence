// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE
class TermModel;
class TermView;
class TermManager;

class TermsWindow final: public QWidget
{
    Q_OBJECT

private:
    TermModel *m_model;
    TermView *m_view;
    TermManager *m_manager;

    QCheckBox *m_check;
    QComboBox *m_combo;

    QPushButton *m_switchButton;
    QPushButton *m_frontButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_backButton;
    QPushButton *m_hideButton;
    QPushButton *m_showButton;
    QPushButton *m_newButton;
    QPushButton *m_closeButton;
    QPushButton *m_stopButton;

private slots:
    void handleManagerAdded(TermManager *manager);
    void handleManagerRemoved(TermManager *manager);
    void handleTitle(const QString &title);

    void handleSync();
    void handleCheckBox();
    void handleIndexChanged();

    void handleSelection();

    void handleSwitch();
    void handleMoveFront();
    void handleMoveUp();
    void handleMoveDown();
    void handleMoveBack();
    void handleHide();
    void handleShow();
    void handleNew();
    void handleClose();
    void handleStop();

protected:
    bool event(QEvent *event);

public:
    TermsWindow(TermManager *manager);

    QSize sizeHint() const { return QSize(800, 480); }

    void bringUp();
};

extern TermsWindow *g_termwin;
