// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "dockwidget.h"
#include "fontbase.h"
#include "url.h"

#include <QRgb>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QLabel;
class QMenu;
QT_END_NAMESPACE
class TermScrollport;
struct TermDirectory;
struct TermFile;
class FileModel;
class FileFilter;
class FileNameItem;
class FileView;
class FileListing;
class FileBanner;
class DynamicMenu;
class DragIcon;

class FileWidget final: public SearchableWidget, public FontBase
{
    Q_OBJECT

private:
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport;

    FileModel *m_model;
    FileFilter *m_filter;
    FileNameItem *m_nameitem;

    QString m_currentDir;
    TermUrl m_currentUrl, m_selectedUrl;
    const TermFile *m_selectedFile = nullptr;
    int m_selected = -1;
    bool m_selecting = false;
    TermFile *m_overrideFile;

    QStackedWidget *m_stack;
    QLabel *m_msgLabel;
    FileView *m_longView;
    FileListing *m_shortView;
    FileBanner *m_banner;

    int m_format;
    int m_formatOverride = 0;

    QString m_linkProfile, m_linkLimit;

    QFont m_font;
    QRect m_dragBounds;
    DragIcon *m_drag = nullptr;

    QMetaObject::Connection m_mocTerm, m_mocUrl;

    void selectItem(int index, const TermFile *file);
    void updateDirectory(const TermDirectory *dir);
    void updateFormat(bool force);
    void updateSize(const QSize &size);

    void addDisplayActions(DynamicMenu *menu);

private slots:
    void handleTermChanged(const TermDirectory *dir);
    void handleDirectoryChanged(const TermDirectory *dir);

    void handleTermActivated(TermInstance *term, TermScrollport *scrollport);
    void handleSettingsChanged();
    void recolor(QRgb bg, QRgb fg);
    void refont(const QFont &font);

    void handleLinkActivated(const QString &link);
    void handleUrlChanged(const TermUrl &tu);

protected:
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

public:
    FileWidget(TermManager *manager);
    ~FileWidget();

    int format() const { return m_formatOverride ? m_formatOverride : m_format; }
    void setActive(bool active);
    void visibilitySet();
    bool hasHeader() const;

    inline bool isCurrentDirectory() const { return !m_currentDir.isEmpty(); }
    inline bool isSelectedFile() const { return m_selectedFile; }
    inline const TermUrl& currentUrl() const { return m_currentUrl; }
    inline const TermUrl& selectedUrl() const { return m_selectedUrl; }
    inline const TermFile* selectedFile() const { return m_selectedFile; }
    inline int selectedIndex() const { return m_selected; }
    bool isRemote() const;

    static void addDisplayActions(DynamicMenu *menu, MainWindow *parent);
    QMenu* getFilePopup(const TermFile *file);
    QMenu* getDirPopup(const QString &path);
    void setFormat(int format);
    void toggleFormat();
    void setSort(int spec);

    void action(int type);
    void action(int type, const QString &dir);
    void toolAction(int index);
    void contextMenu();
    void startDrag(const QString &path);

    // source: 0=external, 1=longview, 2=shortview, 3=termchange, 4=dirchange
    void updateSelection(int row, int source = 2);

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();
    void selectNone();

    void restoreState(int index);
    void saveState(int index);
};
