// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/enums.h"
#include "app/flags.h"
#include "idbase.h"
#include "process.h"
#include "search.h"
#include "settings/palette.h"
#include "settings/alert.h"
#include "lib/types.h"

#include <QHash>
#include <QSet>
#include <QVector>
#include <QSize>
#include <QPoint>
#include <QRgb>
#include <QFont>
#include <memory>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QWidget;
QT_END_NAMESPACE
namespace Tsq { class Unicoding; }
class TermManager;
class TermStack;
class TermBuffers;
class TermScreen;
class TermOverlay;
class ServerInstance;
class ProfileSettings;
class TermKeymap;
class Selection;
class FileTracker;
class ContentTracker;
class Region;
struct TermJob;

struct TermSig
{
    enum Which {
        Nothing = 0, Palette = 1, Font = 2,
        Layout = 4, Fills = 8, Badge = 16, Icon = 32,
        Input = 64, Everything = 0x7f
    };

    unsigned which;
    TermPalette palette;
    QFont font;
    QString layout;
    QString fills;
    QString badge;
    QString icon;

    QString cwd;
    QStringList extraEnv;
    AttributeMap extraAttr;
    Tsq::Uuid id;
};

class TermInstance final: public IdBase
{
    Q_OBJECT
    Q_PROPERTY(QString layout READ layout WRITE setLayout)
    Q_PROPERTY(QString fills READ fills WRITE setFills)
    Q_PROPERTY(QString badge READ badge WRITE setBadge)
    Q_PROPERTY(QString icon READ iconStr WRITE setIcon)
    Q_PROPERTY(int width READ width)
    Q_PROPERTY(int height READ height)

private:
    ServerInstance *m_server;
    ServerInstance *m_peer = nullptr;

    Tsq::TermFlags m_flags;
    std::shared_ptr<Tsq::Unicoding> m_unicoding;
    TermBuffers *m_buffers;
    TermScreen *m_screen;

    ProfileSettings *m_profile;
    TermKeymap *m_keymap = nullptr;
    AlertSettings *m_alert = nullptr;
    AlertNeeds m_needs = NeedsNone;
    short m_flashes = 0;
    bool m_urgent = false;

    QSize m_localSize, m_requestedSize;
    QPoint m_mousePos{ -1, -1 };
    QRgb m_realBg = 0, m_realFg = 0, m_displayBg = 0, m_displayFg = 0;
    TermPalette m_palette;
    QFont m_font, m_displayFont;
    QString m_layout, m_fills, m_badge;

    bool m_updating = false;
    const bool m_isTerm;
    bool m_registered;
    bool m_remoteSizeSeen;
    bool m_localSizeSeen = false;
    bool m_reverseVideo = false;
    bool m_blinkSeen = false;
    bool m_remoteInput = false;
    bool m_wantInput;
    bool m_inputFollower = false;
    bool m_inputLeader = false;

    TermSearch m_search;

    QSet<const TermStack*> m_stacks;
    int m_stackSpec = 0;
    int m_noteNum = 1;

    TermProcess m_process;
    TermOverlay *m_overlay = nullptr;
    FileTracker *m_files;
    ContentTracker *m_content;
    QVector<QString> m_profileStack;

private:
    void reprofile(ProfileSettings *profile);
    void processProfile();
    void setProfileAttributes(AttributeMap &map);
    void setSettingsAttributes(AttributeMap &map, unsigned which);
    void pushSettingsAttributes(unsigned which);

    int calculateChangeType(int type, int level);
    void updateStackSpec();

    void handleEncoding(const std::string &spec);
    void handleOwnership(const QString &ownerIdStr);
    void handlePeerAttribute(const QString &peerIdStr);
    void handleRemoteClipboard(QString key, const QString &value);
    void recolor(QRgb bg, QRgb fg);

    void shutdown();

private slots:
    void handleCommandMode(bool commandMode);
    void handlePresMode(bool presMode);

    void rekeymap(TermKeymap *keymap);
    void handlePaletteChanged(int type);
    void handleFontChanged(int type);
    void handleLayoutChanged(int type);
    void handleFillsChanged(int type);
    void handleBadgeChanged(int type);
    void handleIconChanged(int type);

    void traverseSwitchRules();
    void traverseIconRules();
    void reindicate();

    void handleSettingChanged(const char *property);
    void handleProfileDestroyed();

    void handlePeerDestroyed();

signals:
    void sizeChanged(QSize size);
    void localSizeChanged(QSize size);
    void bellRang();
    void flagsChanged(Tsq::TermFlags flags);
    void ownershipChanged(bool ours);
    void peerChanging();
    void peerChanged();
    void alertChanged();
    void inputChanged();
    void ratelimitChanged(bool ratelimited);

    void processChanged(const QString &key);
    void processExited(QRgb exitColor);

    void profileChanged(const ProfileSettings *profile);
    void keymapChanged(const TermKeymap *keymap);
    void colorsChanged(QRgb bg, QRgb fg);
    void paletteChanged();
    void fontChanged(const QFont &font);
    void layoutChanged(const QString &layoutStr);
    void fillsChanged(const QString &fillsStr);
    void fileSettingsChanged();
    void miscSettingsChanged();

    void stacksChanged();
    void searchChanged(bool searching);
    void contentChanged();

    void cursorHighlight();

public:
    TermInstance(const Tsq::Uuid &id, ServerInstance *server, ProfileSettings *profile,
                 QSize size, bool isTerm, bool ours);
    ~TermInstance();
    void prepare(TermSig *copyfrom, bool duplicate);
    void populateSig(TermSig *copyto);

    inline ServerInstance* server() { return m_server; }
    inline const ServerInstance* server() const { return m_server; }
    inline ServerInstance* peer() { return m_peer; }
    void setPeer(ServerInstance *peer);

    inline Tsq::TermFlags flags() const { return m_flags; }
    void setFlags(Tsq::TermFlags flags);
    inline TermBuffers* buffers() { return m_buffers; }
    inline TermScreen* screen() { return m_screen; }
    inline Tsq::Unicoding* unicoding() const { return m_unicoding.get(); }
    inline auto unicodingPtr() const { return m_unicoding; }

    inline ProfileSettings* profile() { return m_profile; }
    inline TermKeymap* keymap() { return m_keymap; }
    inline AlertSettings* alert() { return m_alert; }
    const QString& profileName() const;
    void setProfile(ProfileSettings *profile);
    void pushProfile(ProfileSettings *profile);
    void popProfile();
    void setAlert(AlertSettings *alert);

    int width() const;
    int height() const;
    bool setLocalSize(QSize size);
    void setRemoteSize(QSize size);
    inline QPoint mousePos() const { return m_mousePos; }
    void setMousePos(QPoint mousePos);

    inline QRgb realBg() const { return m_realBg; }
    inline QRgb realFg() const { return m_realFg; }
    inline QRgb bg() const { return m_displayBg; }
    inline QRgb fg() const { return m_displayFg; }
    inline const TermPalette& palette() const { return m_palette; }
    void setPalette(const TermPalette &palette);
    inline const QFont& realFont() const { return m_font; }
    inline const QFont& font() const { return m_displayFont; }
    void setFont(const QFont &font);
    inline const QString& layout() const { return m_layout; }
    void setLayout(const QString &layoutStr);
    inline const QString& fills() const { return m_fills; }
    void setFills(const QString &fillsStr);
    inline const QString& badge() const { return m_badge; }
    void setBadge(const QString &badge);
    void setIcon(const QString &icon);
    inline bool remoteInput() const { return m_remoteInput; }
    void setRemoteInput(bool wantInput);

    inline bool updating() const { return m_updating; }
    void beginUpdate();
    inline void endUpdate() { m_updating = false; }
    inline bool isTerm() const { return m_isTerm; }
    inline bool registered() const { return m_registered; }
    void setRegistered();
    inline bool blinkSeen() const { return m_blinkSeen; }

    inline const TermSearch& search() const { return m_search; }
    inline bool searching() const { return m_search.active; }
    void setSearch(TermSearch &search);

    inline const auto& stacks() const { return m_stacks; }
    inline int stackCount() const { return m_stacks.size(); }
    void addStack(const TermStack *stack);
    void removeStack(const TermStack *stack);

    inline int noteNum() const { return m_noteNum; }
    inline void incNoteNum() { ++m_noteNum; }
    inline void resetNoteNum() { m_noteNum = 1; }

    QString title() const;
    QString name() const;
    inline const TermProcess* process() const { return &m_process; }
    inline bool overlayActive() const { return m_overlay; }
    inline FileTracker* files() { return m_files; }
    void hideOverlay();

    inline ContentTracker* content() { return m_content; }
    void registerImage(const Region *region);
    void updateImage(const QString &id, const char *data, size_t len);
    void fetchImage(const QString &id, TermManager *manager);
    void putImage(const Region *r);

    void setAttribute(const QString &key, const QString &value);
    void removeAttribute(const QString &key);

    void reportBellRang();
    void pushInput(TermManager *manager, const QString &text, bool bracketed = true);
    bool keyPressEvent(TermManager *manager, QKeyEvent *event, Tsq::TermFlags flags);
    void mouseEvent(Tsq::MouseEventFlags flags, const QPoint &p);

    void removeAnnotation(regionid_t id);
    void takeOwnership();
    void close(TermManager *manager);

    inline bool alertNeedsActivity() const { return m_needs == NeedsActivity; }
    inline bool alertNeedsJobs() const { return m_needs == NeedsJobs; }
    inline bool alertNeedsContent() const { return m_needs == NeedsContent; }
    inline bool alertNeedsBell() const { return m_needs == NeedsBell; }
    inline bool alertNeedsAttr() const { return m_needs == NeedsAttr; }
    inline bool alertUrgent() const { return m_urgent; }
    inline short alertFlashes() const { return m_flashes; }
    void reportAlert();
    void clearAlert();
    void reportJob(const TermJob *job);

    inline bool inputFollower() const { return m_inputFollower; }
    void setInputFollower(bool inputFollower);
    inline bool inputLeader() const { return m_inputLeader; }
    void setInputLeader(bool inputLeader);
};

inline void
TermInstance::beginUpdate()
{
    m_updating = true;
    if (alertNeedsActivity())
        m_alert->reportActivity(this);
}

inline void
TermInstance::reportBellRang()
{
    if (alertNeedsBell())
        m_alert->reportBell(this);
    emit bellRang();
}

inline void
TermInstance::clearAlert()
{
    if (m_urgent) {
        m_flashes = 0;
        m_urgent = false;
        emit alertChanged();
        reindicate();
    }
}

inline void
TermInstance::addStack(const TermStack *stack)
{
    m_stacks.insert(stack);
    updateStackSpec();
}

inline void
TermInstance::removeStack(const TermStack *stack)
{
    m_stacks.remove(stack);
    updateStackSpec();
}
