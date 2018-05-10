// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>
#include <QMap>
#include <QVector>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableView;
class QStandardItemModel;
QT_END_NAMESPACE
struct SettingDef;
class SettingsBase;

class StartupDialog final: public QDialog
{
    Q_OBJECT

private:
    const SettingDef *m_def;
    SettingsBase *m_settings;

    QVector<std::pair<QString,QIcon>> m_profiles, m_select;
    QStandardItemModel *m_profileModel, *m_selectModel;
    QTableView *m_profileView, *m_selectView;
    QMap<int,int> m_profileRows, m_selectRows; // sorted

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_clearButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;

    void addDefault(const QString &str);

private slots:
    void handleProfileSelect();
    void handleSelectSelect();

    void handleAddServer();
    void handleAddGlobal();
    void handleAdd();
    void handleRemove();
    void handleClear();
    void handleMoveUp();
    void handleMoveDown();
    void handleReset();

    void handleAccept();

protected:
    bool event(QEvent *event);

public:
    StartupDialog(const SettingDef *def, SettingsBase *settings, QWidget *parent);

    QSize sizeHint() const { return QSize(640, 480); }

    void bringUp();
};
