// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "term.h"
#include "stack.h"
#include "manager.h"
#include "listener.h"
#include "buffers.h"
#include "screen.h"
#include "scrollport.h"
#include "overlay.h"
#include "server.h"
#include "termformat.h"
#include "filetracker.h"
#include "pastetask.h"
#include "imagetask.h"
#include "contentmodel.h"
#include "fontbase.h"
#include "mainwindow.h"
#include "thumbicon.h"
#include "job.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/servinfo.h"
#include "settings/keymap.h"
#include "settings/profile.h"
#include "settings/switchrule.h"
#include "settings/iconrule.h"
#include "lib/sequences.h"
#include "lib/unicode.h"

#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>

#define TR_ASK1 TL("question", "Confirm close. Close terminal?")
#define TR_ASK2 TL("question", "Process is still running in terminal. Close terminal?")
#define TR_ASK3 TL("question", "Foreground job may be active in terminal. Close terminal?")
#define TR_TITLE1 TL("window-title", "Confirm Close")

TermInstance::TermInstance(const Tsq::Uuid &id, ServerInstance *server, ProfileSettings *profile,
                           QSize size, bool isTerm, bool ours) :
    IdBase(id, ours, server),
    m_server(server),
    m_unicoding(g_settings->defaultEncoding()),
    m_profile(profile),
    m_palette(profile->content()),
    m_layout(profile->layout()),
    m_fills(profile->fills()),
    m_badge(profile->badge()),
    m_isTerm(isTerm),
    m_registered(!ours),
    m_remoteSizeSeen(ours || !isTerm),
    m_wantInput(profile->remoteInput())
{
    m_push = m_registered;
    m_flags = g_listener->commandMode() ? Tsqt::CommandMode : 0;
    connect(server, SIGNAL(connectionChanged()), SLOT(reindicate()));
    connect(g_listener, SIGNAL(commandModeChanged(bool)), SLOT(handleCommandMode(bool)));
    connect(g_listener, SIGNAL(presModeChanged(bool)), SLOT(handlePresMode(bool)));
    connect(g_global, SIGNAL(iconRulesChanged()), SLOT(traverseIconRules()));
    connect(g_global, SIGNAL(avatarsChanged()), SIGNAL(indicatorsChanged()));

    if (m_remoteSizeSeen)
        m_requestedSize = size;

    m_buffers = new TermBuffers(this);
    m_screen = new TermScreen(this, size, this);
    m_format = new TermFormat(this);
    m_files = new FileTracker(this);
    m_content = new ContentTracker(this);

    m_iconTypes[0] = ThumbIcon::CommandType;
    m_iconTypes[1] = ThumbIcon::TerminalType;
    m_indicatorTypes[1] = m_indicatorTypes[0] = ThumbIcon::InvalidType;

    reprofile(profile);
    recolor(m_palette.bg(), m_palette.fg());
    reindicate();
    m_icons[1] = profile->icon();
    m_font.fromString(profile->font());
    m_displayFont = m_font;
    if (g_listener->presMode())
        m_displayFont.setPointSize(m_font.pointSize() + g_global->presFontSize());
}

TermInstance::~TermInstance()
{
    if (m_alert) {
        m_alert->unwatch(this);
        m_alert->putReference();
    }
    m_profile->putReference();
}

void
TermInstance::setSettingsAttributes(AttributeMap &map, unsigned which)
{
    // write/read
    if (which & TermSig::Palette) {
        map[g_attr_PREF_PALETTE] = map[g_attr_SESSION_PALETTE] = m_palette.tStr();
        map[g_attr_PREF_DIRCOLORS] = map[g_attr_SESSION_DIRCOLORS] = m_palette.dStr();
    }
    if (which & TermSig::Font)
        map[g_attr_PREF_FONT] = map[g_attr_SESSION_FONT] = m_font.toString();
    if (which & TermSig::Layout)
        map[g_attr_PREF_LAYOUT] = map[g_attr_SESSION_LAYOUT] = m_layout;
    if (which & TermSig::Fills)
        map[g_attr_PREF_FILLS] = map[g_attr_SESSION_FILLS] = m_fills;
    if (which & TermSig::Badge)
        map[g_attr_PREF_BADGE] = map[g_attr_SESSION_BADGE] = m_badge;
    if (which & TermSig::Icon)
        map[g_attr_SESSION_ICON] = m_icons[1];
    // write-only
    if (which & TermSig::Input)
        map[g_attr_PREF_INPUT] = '0' + m_wantInput;
}

void
TermInstance::setProfileAttributes(AttributeMap &map)
{
    // write/read
    map[g_attr_PREF_PALETTE] = map[g_attr_SESSION_PALETTE] = m_profile->palette();
    map[g_attr_PREF_DIRCOLORS] = map[g_attr_SESSION_DIRCOLORS] = m_profile->dircolors();
    map[g_attr_PREF_FONT] = map[g_attr_SESSION_FONT] = m_profile->font();
    map[g_attr_PREF_LAYOUT] = map[g_attr_SESSION_LAYOUT] = m_profile->layout();
    map[g_attr_PREF_FILLS] = map[g_attr_SESSION_FILLS] = m_profile->fills();
    map[g_attr_PREF_BADGE] = map[g_attr_SESSION_BADGE] = m_profile->badge();
    map[g_attr_SESSION_ICON] = m_profile->icon();
    // write-only
    map[g_attr_PREF_INPUT] = '0' + m_profile->remoteInput();
}

void
TermInstance::populateSig(TermSig *copyto)
{
    copyto->which = TermSig::Everything;
    copyto->palette = m_palette;
    copyto->font = m_font;
    copyto->layout = m_layout;
    copyto->fills = m_fills;
    copyto->badge = m_badge;
    copyto->icon = m_icons[1];
    copyto->cwd = m_process.workingDir;
    copyto->id = id();
}

void
TermInstance::prepare(TermSig *copyfrom, bool duplicate)
{
    QStringList extraEnv;

    m_profile->toAttributes(m_attributes);
    m_attributes[g_attr_SESSION_TITLE] = m_profile->name();
    m_attributes[g_attr_SESSION_TITLE2] = m_profile->name();

    if (duplicate)
        m_attributes[g_attr_PREF_STARTDIR] = copyfrom->cwd;
    else if (m_profile->sameDirectory())
        m_attributes[g_attr_PREF_STARTDIR] = m_server->cwd();

    if (m_profile->lang().isEmpty())
        m_attributes[g_attr_PREF_LANG] = g_settings->defaultLang();
    else if (m_profile->lang() != g_str_CURRENT_PROFILE)
        m_attributes[g_attr_PREF_LANG] = m_profile->lang();

    if (copyfrom) {
        if (copyfrom->which & TermSig::Palette)
            setPalette(copyfrom->palette);
        if (copyfrom->which & TermSig::Font)
            setFont(copyfrom->font);
        if (copyfrom->which & TermSig::Layout)
            setLayout(copyfrom->layout);
        if (copyfrom->which & TermSig::Fills)
            setFills(copyfrom->fills);
        if (copyfrom->which & TermSig::Badge)
            setBadge(copyfrom->badge);
        if (copyfrom->which & TermSig::Icon)
            setIcon(copyfrom->icon);

        extraEnv = std::move(copyfrom->extraEnv);

        const AttributeMap &extraAttr = copyfrom->extraAttr;
        for (auto i = extraAttr.begin(), j = extraAttr.end(); i != j; ++i)
            m_attributes.insert(i.key(), *i);
    }

    setSettingsAttributes(m_attributes, TermSig::Everything);

    if (m_profile->dircolorsEnv()) {
        extraEnv.append(m_profile->content().envStr());
        extraEnv.append(A("+USER_LS_COLORS=yes"));
    }
    if (m_profile->langEnv()) {
        auto i = m_attributes.constFind(g_attr_PREF_LANG);
        if (i != m_attributes.cend())
            extraEnv.append(A("+LANG=") + *i);
    }

    if (!extraEnv.isEmpty())
        m_attributes[g_attr_PREF_ENVIRON] = extraEnv.join('\x1f');
}

inline void
TermInstance::pushSettingsAttributes(unsigned which)
{
    if (m_ours && m_push) {
        AttributeMap map;
        setSettingsAttributes(map, which);
        g_listener->pushTermAttributes(this, map);
    }
}

void
TermInstance::setRegistered()
{
    m_registered = true;
    m_push = true;
    emit ownershipChanged(true);
}

void
TermInstance::recolor(QRgb bg, QRgb fg)
{
    m_realBg = bg;
    m_realFg = fg;
    QRgb displayBg = m_reverseVideo ? m_realFg : m_realBg;
    QRgb displayFg = m_reverseVideo ? m_realBg : m_realFg;

    if (m_displayBg != displayBg || m_displayFg != displayFg) {
        m_displayBg = displayBg;
        m_displayFg = displayFg;
        emit colorsChanged(m_displayBg, m_displayFg);
    }
}

void
TermInstance::setPalette(const TermPalette &palette)
{
    if (m_palette != palette) {
        m_palette = palette;
        emit paletteChanged();
        recolor(m_palette.bg(), m_palette.fg());
        pushSettingsAttributes(TermSig::Palette);
    }
}

inline int
TermInstance::calculateChangeType(int type, int level)
{
    if (!m_isTerm)
        return ChProf;
    if (type == ChLevel)
        return level;
    if ((type == ChProf && m_ours) || type == level)
        return type;

    return -1;
}

void
TermInstance::handlePaletteChanged(int type)
{
    QString tc, dc;

    switch (calculateChangeType(type, m_profile->remoteColors())) {
    case ChSess:
        tc = m_attributes.value(g_attr_SESSION_PALETTE, m_profile->palette());
        dc = m_attributes.value(g_attr_SESSION_DIRCOLORS, m_profile->dircolors());
        setPalette(TermPalette(tc, dc));
        break;
    case ChPref:
        tc = m_attributes.value(g_attr_PREF_PALETTE, m_profile->palette());
        dc = m_attributes.value(g_attr_PREF_DIRCOLORS, m_profile->dircolors());
        setPalette(TermPalette(tc, dc));
        break;
    case ChProf:
        setPalette(m_profile->content());
    }
}

void
TermInstance::setFont(const QFont &font)
{
    if (m_font != font) {
        m_displayFont = m_font = font;

        if (g_listener->presMode())
            m_displayFont.setPointSize(font.pointSize() + g_global->presFontSize());

        emit fontChanged(m_displayFont);
        pushSettingsAttributes(TermSig::Font);
    }
}

void
TermInstance::handleFontChanged(int type)
{
    QFont font;
    AttributeMap::const_iterator i, j = m_attributes.cend();

    switch (calculateChangeType(type, m_profile->remoteFont())) {
    case ChSess:
        i = m_attributes.constFind(g_attr_SESSION_FONT);
        if (i != j && font.fromString(*i) && FontBase::isFixedWidth(font))
            setFont(font);
        break;
    case ChPref:
        i = m_attributes.constFind(g_attr_PREF_FONT);
        if (i != j && font.fromString(*i) && FontBase::isFixedWidth(font))
            setFont(font);
        break;
    case ChProf:
        font.fromString(m_profile->font());
        setFont(font);
    }
}

void
TermInstance::setLayout(const QString &layoutStr)
{
    if (m_layout != layoutStr) {
        emit layoutChanged(m_layout = layoutStr);
        pushSettingsAttributes(TermSig::Layout);
    }
}

void
TermInstance::handleLayoutChanged(int type)
{
    switch (calculateChangeType(type, m_profile->remoteLayout())) {
    case ChSess:
        setLayout(m_attributes.value(g_attr_SESSION_LAYOUT, m_profile->layout()));
        break;
    case ChPref:
        setLayout(m_attributes.value(g_attr_PREF_LAYOUT, m_profile->layout()));
        break;
    case ChProf:
        setLayout(m_profile->layout());
    }
}

void
TermInstance::setFills(const QString &fillsStr)
{
    if (m_fills != fillsStr) {
        emit fillsChanged(m_fills = fillsStr);
        pushSettingsAttributes(TermSig::Fills);
    }
}

void
TermInstance::handleFillsChanged(int type)
{
    switch (calculateChangeType(type, m_profile->remoteFills())) {
    case ChSess:
        setFills(m_attributes.value(g_attr_SESSION_FILLS, m_profile->fills()));
        break;
    case ChPref:
        setFills(m_attributes.value(g_attr_PREF_FILLS, m_profile->fills()));
        break;
    case ChProf:
        setFills(m_profile->fills());
    }
}

void
TermInstance::setBadge(const QString &badge)
{
    if (m_badge != badge) {
        m_format->reportBadgeFormat(m_badge = badge);
        pushSettingsAttributes(TermSig::Badge);
    }
}

void
TermInstance::handleBadgeChanged(int type)
{
    switch (calculateChangeType(type, m_profile->remoteBadge())) {
    case ChSess:
        setBadge(m_attributes.value(g_attr_SESSION_BADGE, m_profile->badge()));
        break;
    case ChPref:
        setBadge(m_attributes.value(g_attr_PREF_BADGE, m_profile->badge()));
        break;
    case ChProf:
        setBadge(m_profile->badge());
    }
}

void
TermInstance::setIcon(const QString &icon)
{
    if (m_icons[1] != icon) {
        m_icons[1] = icon;
        emit iconsChanged(m_icons, m_iconTypes);
        pushSettingsAttributes(TermSig::Icon);
    }
}

void
TermInstance::handleIconChanged(int type)
{
    switch (calculateChangeType(type, ChSess)) {
    case ChSess:
        setIcon(m_attributes.value(g_attr_SESSION_ICON, m_profile->icon()));
        break;
    case ChProf:
        setIcon(m_profile->icon());
        break;
    }
}

void
TermInstance::setRemoteInput(bool wantInput)
{
    if (m_wantInput != wantInput) {
        m_wantInput = wantInput;
        pushSettingsAttributes(TermSig::Input);
    }
}

void
TermInstance::rekeymap(TermKeymap *keymap)
{
    if (m_keymap)
        m_keymap->reset();

    emit keymapChanged(m_keymap = keymap);

    if (m_keymap)
        m_keymap->reset();
}

void
TermInstance::reprofile(ProfileSettings *profile)
{
    m_profile = profile;
    m_profile->takeReference();
    m_profile->activate();
    rekeymap(m_profile->keymap());

    connect(m_profile, SIGNAL(destroyed()), SLOT(handleProfileDestroyed()));
    connect(m_profile, SIGNAL(keymapChanged(TermKeymap*)), SLOT(rekeymap(TermKeymap*)));
    connect(m_profile, SIGNAL(paletteChanged(int)), SLOT(handlePaletteChanged(int)));
    connect(m_profile, SIGNAL(remoteColorsChanged(int)), SLOT(handlePaletteChanged(int)));
    connect(m_profile, SIGNAL(fontChanged(int)), SLOT(handleFontChanged(int)));
    connect(m_profile, SIGNAL(remoteFontChanged(int)), SLOT(handleFontChanged(int)));
    connect(m_profile, SIGNAL(layoutChanged(int)), SLOT(handleLayoutChanged(int)));
    connect(m_profile, SIGNAL(remoteLayoutChanged(int)), SLOT(handleLayoutChanged(int)));
    connect(m_profile, SIGNAL(fillsChanged(int)), SLOT(handleFillsChanged(int)));
    connect(m_profile, SIGNAL(remoteFillsChanged(int)), SLOT(handleFillsChanged(int)));
    connect(m_profile, SIGNAL(badgeChanged(int)), SLOT(handleBadgeChanged(int)));
    connect(m_profile, SIGNAL(remoteBadgeChanged(int)), SLOT(handleBadgeChanged(int)));
    connect(m_profile, SIGNAL(iconChanged(int)), SLOT(handleIconChanged(int)));
    connect(m_profile, &ProfileSettings::remoteInputChanged, this, &TermInstance::setRemoteInput);
    connect(m_profile, SIGNAL(miscSettingsChanged()), SIGNAL(miscSettingsChanged()));
    connect(m_profile, SIGNAL(fileSettingsChanged()), SIGNAL(fileSettingsChanged()));
    connect(m_profile, SIGNAL(settingChanged(const char*,QVariant)), SLOT(handleSettingChanged(const char*)));
}

void
TermInstance::processProfile()
{
    handleFontChanged(ChProf);
    handleLayoutChanged(ChProf);
    handleFillsChanged(ChProf);
    handlePaletteChanged(ChProf);
    handleBadgeChanged(ChProf);
    handleIconChanged(ChProf);
    setRemoteInput(m_profile->remoteInput());

    emit miscSettingsChanged();
    emit fileSettingsChanged();
    emit profileChanged(m_profile);

    /* Push attributes if we own this terminal */
    if (m_ours) {
        AttributeMap map;
        m_profile->toAttributes(map);
        setProfileAttributes(map);
        g_listener->pushTermAttributes(this, map);
    }
}

void
TermInstance::handleProfileDestroyed()
{
    reprofile(g_settings->defaultProfile());
    processProfile();
}

void
TermInstance::setProfile(ProfileSettings *profile)
{
    if (m_profile != profile) {
        m_profile->disconnect(this);
        m_profile->putReference();

        reprofile(profile);
        processProfile();
    }
}

void
TermInstance::pushProfile(ProfileSettings *profile)
{
    if (m_profileStack.size() == PROFILE_STACK_MAX)
        m_profileStack.pop_front();

    m_profileStack.push_back(m_profile->name());
    setProfile(profile);
}

void
TermInstance::popProfile()
{
    QString profileName;

    if (m_profileStack.isEmpty()) {
        profileName = g_str_CURRENT_PROFILE;
    } else {
        profileName = m_profileStack.back();
        m_profileStack.pop_back();
    }

    setProfile(g_settings->profile(profileName));
}

void
TermInstance::reindicate()
{
    QString saved[2] = { m_indicators[0], m_indicators[1] };
    int savedTypes[2] = { m_indicatorTypes[0], m_indicatorTypes[1] };

    if (!m_server->conn()) {
        m_indicators[0] = A("disconnected");
        m_indicatorTypes[0] = ThumbIcon::IndicatorType;
        m_indicatorTypes[1] = ThumbIcon::InvalidType;
        goto out;
    }

    m_indicatorTypes[0] = ThumbIcon::IndicatorType;
    m_indicatorTypes[1] = ThumbIcon::IndicatorType;

    if (m_urgent) {
        m_indicators[1] = A("urgent");
    }
    else if (m_flags & (Tsq::SoftScrollLock|Tsq::HardScrollLock)) {
        m_indicators[1] = A("locked");
    }
    else if (m_inputLeader) {
        m_indicators[1] = A("leader");
    }
    else if (m_inputFollower) {
        m_indicators[1] = A("follower");
    }
    else if (m_alert) {
        m_indicators[1] = A("alert");
    }
    else {
        m_indicatorTypes[1] = ThumbIcon::InvalidType;
    }

    if (m_peer) {
        m_indicators[0] = A("connected");
    }
    else if (!m_ours) {
        m_indicatorTypes[0] = ThumbIcon::AvatarType;
        m_indicators[0] = m_attributes.value(g_attr_OWNER_ID);
    }
    else {
        m_indicatorTypes[0] = ThumbIcon::InvalidType;
    }

    if (m_indicators[0] != saved[0] || m_indicators[1] != saved[1] ||
        m_indicatorTypes[0] != savedTypes[0] || m_indicatorTypes[1] != savedTypes[1])
    out:
        emit indicatorsChanged();
}

void
TermInstance::setAlert(AlertSettings *alert)
{
    if (m_alert) {
        m_alert->unwatch(this);
        m_alert->putReference();
    }

    if ((m_alert = alert)) {
        alert->takeReference();
        m_needs = alert->needs();
        m_flashes = 0;
        m_urgent = false;
        alert->watch(this);
    } else {
        m_needs = NeedsNone;
    }

    emit alertChanged();
    reindicate();
}

void
TermInstance::reportAlert()
{
    auto *manager = g_listener->activeManager();
    if (manager) {
        if (m_alert->actServer())
            manager->actionReorderServerFirst(m_server->idStr());
        if (m_alert->actTerm())
            manager->actionReorderTerminalFirst(idStr());
        if (m_alert->actInd())
            m_urgent = true;
        if (m_alert->actFlash())
            m_flashes = m_alert->paramFlashes();
        if (m_alert->actNotify())
            manager->actionNotifySend(m_alert->name(), m_alert->paramMessage());
        if (m_alert->actPush())
            pushProfile(g_settings->profile(m_alert->paramProfile()));
        if (m_alert->actSwitch())
            setProfile(g_settings->profile(m_alert->paramProfile()));
        if (m_alert->actSlot() && !m_alert->paramSlot().isEmpty())
            manager->invokeSlot(m_alert->slotStr(this));
        if (m_alert->actLaunch())
            manager->actionLaunchCommand(m_alert->paramLauncher(), m_server->idStr(),
                                         g_mtstr);
    }

    setAlert(nullptr);
}

void
TermInstance::reportJob(const TermJob *job)
{
    if (m_profile->exitEffect() && m_ours)
        if (job->duration >= m_profile->exitRuntime())
            emit processExited(job->exitColor());

    // Note: order matters, do this after exit report
    if (alertNeedsJobs())
        m_alert->reportJob(this, job);
}

void
TermInstance::handleSettingChanged(const char *property)
{
    /* Push attribute if we own this terminal */
    if (m_ours) {
        auto elt = m_profile->toAttribute(property);
        g_listener->pushTermAttribute(this, elt.first, elt.second);
    }
}

void
TermInstance::handleEncoding(const std::string &specStr)
{
    Tsq::UnicodingSpec spec(specStr);
    if (spec == m_unicoding->spec())
        return;

    auto *unicoding = Tsq::Unicoding::create(spec);
    m_unicoding.reset(unicoding);

    if (!m_profile->encodingWarn())
        return;

    auto ourStr = unicoding->spec().name();
    if (ourStr != specStr) {
        QString theirs = QString::fromStdString(specStr);
        theirs.replace('\x1f', ' ');
        QString ours = QString::fromStdString(ourStr);
        ours.replace('\x1f', ' ');

        qCWarning(lcSettings) << "Warning: unsupported encoding" << theirs
                              << "(closest match is" << ours << ")";
    }
}

void
TermInstance::handleOwnership(const QString &ownerIdStr)
{
    Tsq::Uuid id(ownerIdStr.toStdString());
    bool ours = (id == g_listener->id());

    if (!ours && id && g_settings->needAvatar(ownerIdStr))
        g_listener->pushServerAvatarRequest(m_server, id);

    if (m_ours != ours) {
        if ((m_ours = ours)) {
            /* Request a resize */
            if (m_localSizeSeen)
                setLocalSize(m_localSize);
            else
                m_requestedSize = m_screen->size();

            /* Push attributes */
            AttributeMap map;
            m_profile->toAttributes(map);
            setSettingsAttributes(map, TermSig::Everything);
            g_listener->pushTermAttributes(this, map);
        }
        emit ownershipChanged(ours);
        reindicate();
    }
}

void
TermInstance::takeOwnership()
{
    g_listener->pushTermOwnership(this);

    if (m_profile->remoteReset())
    {
        AttributeMap map;
        setProfileAttributes(map);
        g_listener->pushTermAttributes(this, map);
    }
}

void
TermInstance::hideOverlay()
{
    if (m_overlay && m_isTerm) {
        m_buffers->beginSetOverlay(nullptr);
        m_overlay->deleteLater();
        m_overlay = nullptr;
        m_buffers->endSetOverlay();
    }
}

void
TermInstance::setPeer(ServerInstance *peer)
{
    if (m_peer) {
        disconnect(m_peer, SIGNAL(destroyed()), this, SLOT(handlePeerDestroyed()));
        emit peerChanging();
    }

    // cannot be nullptr
    m_peer = peer;
    connect(m_peer, SIGNAL(destroyed()), SLOT(handlePeerDestroyed()));
    emit peerChanged();
    reindicate();
}

void
TermInstance::handlePeerAttribute(const QString &peerIdStr)
{
    auto *server = g_listener->lookupServer(peerIdStr.toStdString());
    if (server)
        setPeer(server);

    if (!m_overlay) {
        TermOverlay *overlay = new TermOverlay(this);
        m_buffers->beginSetOverlay(overlay);
        m_overlay = overlay;
        m_buffers->endSetOverlay();
    }
}

void
TermInstance::handlePeerDestroyed()
{
    m_peer = nullptr;
    hideOverlay();
    emit peerChanged();
    reindicate();
}

void
TermInstance::handleRemoteClipboard(QString key, const QString &value)
{
    if (m_ours && m_profile->remoteClipboard())
    {
        key.remove(0, sizeof(TSQ_ATTR_CLIPBOARD_PREFIX) - 1);
        QString text;
        bool select;
        auto *manager = g_listener->activeManager();

        if (key == A("c")) {
            text = QByteArray::fromBase64(value.toLatin1());
            QApplication::clipboard()->setText(text);
            select = false;
        } else if (key == A("p")) {
            text = QByteArray::fromBase64(value.toLatin1());
            QApplication::clipboard()->setText(text, QClipboard::Selection);
            select = true;
        } else {
            manager = nullptr;
        }

        if (manager)
            manager->reportClipboardCopy(text.size(), CopiedChars, select, true);
    }
}

void
TermInstance::traverseIconRules()
{
    QString ti = g_iconrules->traverse(m_attributes);
    if (m_icons[0] != ti) {
        m_icons[0] = ti;
        emit iconsChanged(m_icons, m_iconTypes);
    }
}

void
TermInstance::traverseSwitchRules()
{
    QString tp;
    switch (g_switchrules->traverse(m_attributes, m_profile->name(), tp)) {
    case ToVerbSwitch:
        setProfile(g_settings->profile(tp));
        break;
    case ToVerbPush:
        pushProfile(g_settings->profile(tp));
        break;
    case ToVerbPop:
        popProfile();
        break;
    default:
        break;
    }
}

void
TermInstance::setAttribute(const QString &key, const QString &value)
{
    auto i = m_attributes.find(key);
    if (i == m_attributes.cend())
        m_attributes.insert(key, value);
    else if (*i != value)
        *i = value;
    else
        return;

    if (g_switchrules->hasVariable(key))
        traverseSwitchRules();
    if (g_iconrules->hasVariable(key))
        traverseIconRules();

    if (key.startsWith(g_attr_PROC_PREFIX)) {
        m_process.setAttribute(key, value);
        emit processChanged(key);
    }
    else if (key == g_attr_OWNER_ID) {
        handleOwnership(value);
    }
    else if (key == g_attr_PEER) {
        handlePeerAttribute(value);
    }
    else if (key == g_attr_ENCODING) {
        handleEncoding(value.toStdString());
    }
    else if (key.startsWith(g_attr_PREF_PREFIX)) {
        m_push = false;
        if (key == g_attr_PREF_BADGE)
            handleBadgeChanged(ChPref);
        else if (key == g_attr_PREF_PALETTE || key == g_attr_PREF_DIRCOLORS)
            handlePaletteChanged(ChPref);
        else if (key == g_attr_PREF_FONT)
            handleFontChanged(ChPref);
        else if (key == g_attr_PREF_LAYOUT)
            handleLayoutChanged(ChPref);
        else if (key == g_attr_PREF_FILLS)
            handleFillsChanged(ChPref);
        else if (key == g_attr_PREF_INPUT)
            m_remoteInput = value == A("1");
        m_push = true;
    }
    else if (key.startsWith(g_attr_SESSION_PREFIX)) {
        m_push = false;
        if (key == g_attr_SESSION_BADGE)
            handleBadgeChanged(ChSess);
        else if (key == g_attr_SESSION_PALETTE || key == g_attr_SESSION_DIRCOLORS)
            handlePaletteChanged(ChSess);
        else if (key == g_attr_SESSION_FONT)
            handleFontChanged(ChSess);
        else if (key == g_attr_SESSION_LAYOUT)
            handleLayoutChanged(ChSess);
        else if (key == g_attr_SESSION_FILLS)
            handleFillsChanged(ChSess);
        else if (key == g_attr_SESSION_ICON)
            handleIconChanged(ChSess);
        m_push = true;
    }
    else if (key.startsWith(g_attr_CLIPBOARD_PREFIX)) {
        handleRemoteClipboard(key, value);
    }

    if (alertNeedsAttr())
        m_alert->reportAttribute(this, key, value);

    emit attributeChanged(key, value);
}

void
TermInstance::removeAttribute(const QString &key)
{
    if (!m_attributes.remove(key))
        return;

    if (g_switchrules->hasVariable(key))
        traverseSwitchRules();
    if (g_iconrules->hasVariable(key))
        traverseIconRules();

    if (key.startsWith(g_attr_PROC_PREFIX)) {
        m_process.clearAttribute(key);
        emit processChanged(key);
    }
    else if (key == g_attr_PEER) {
        handlePeerDestroyed();
    }
    else if (key.startsWith(g_attr_PREF_PREFIX)) {
        m_push = false;
        if (key == g_attr_PREF_BADGE)
            handleBadgeChanged(ChPref);
        else if (key == g_attr_PREF_PALETTE || key == g_attr_PREF_DIRCOLORS)
            handlePaletteChanged(ChPref);
        else if (key == g_attr_PREF_FONT)
            handleFontChanged(ChPref);
        else if (key == g_attr_PREF_LAYOUT)
            handleLayoutChanged(ChPref);
        else if (key == g_attr_PREF_FILLS)
            handleFillsChanged(ChPref);
        m_push = true;
    }
    else if (key.startsWith(g_attr_SESSION_PREFIX)) {
        m_push = false;
        if (key == g_attr_SESSION_BADGE)
            handleBadgeChanged(ChSess);
        else if (key == g_attr_SESSION_PALETTE || key == g_attr_SESSION_DIRCOLORS)
            handlePaletteChanged(ChSess);
        else if (key == g_attr_SESSION_FONT)
            handleFontChanged(ChSess);
        else if (key == g_attr_SESSION_LAYOUT)
            handleLayoutChanged(ChSess);
        else if (key == g_attr_SESSION_FILLS)
            handleFillsChanged(ChSess);
        else if (key == g_attr_SESSION_ICON)
            handleIconChanged(ChSess);
        m_push = true;
    }
    emit attributeRemoved(key);
}

void
TermInstance::setFlags(Tsq::TermFlags flags)
{
    flags &= ~Tsq::LocalTermMask;
    flags |= m_flags & Tsq::LocalTermMask;

    if (m_flags != flags) {
        if ((m_flags & Tsq::MouseModeMask) != (flags & Tsq::MouseModeMask)) {
            m_mousePos = QPoint(-1, -1);
        }
        bool rlim = (m_flags & Tsq::RateLimited) != (flags & Tsq::RateLimited);

        m_flags = flags;

        if (rlim)
            emit ratelimitChanged(flags & Tsq::RateLimited);

        m_blinkSeen = (flags & Tsq::BlinkSeen);
        bool reverseVideo = (flags & Tsq::ReverseVideo);
        if (m_reverseVideo != reverseVideo) {
            m_reverseVideo = reverseVideo;
            recolor(m_realBg, m_realFg);
        }

        emit flagsChanged(flags);
        reindicate();
    }
}

void
TermInstance::handleCommandMode(bool commandMode)
{
    if (commandMode)
        m_flags |= Tsqt::CommandMode;
    else
        m_flags &= ~Tsqt::CommandMode;

    emit flagsChanged(m_flags);
}

void
TermInstance::handlePresMode(bool presMode)
{
    int size = (presMode ? 1 : -1) * g_global->presFontSize();
    if (size) {
        m_displayFont.setPointSize(m_displayFont.pointSize() + size);
        emit fontChanged(m_displayFont);
    }
}

int
TermInstance::width() const
{
    return m_screen->width();
}

int
TermInstance::height() const
{
    return m_screen->height();
}

bool
TermInstance::setLocalSize(QSize size)
{
    // qCDebug(lcLayout) << this << "in setLocalSize, arg" << size;

    if (m_ours) {
        if (m_remoteSizeSeen && m_requestedSize != size) {
            m_requestedSize = size;
            g_listener->pushTermResize(this, size);
            qCDebug(lcLayout) << this << "requesting size" << size << "(local)";
        }
        else if (!m_remoteSizeSeen) {
            m_requestedSize = size;
        }
    }

    if (m_localSize == size)
        return false;

    m_localSize = size;
    m_localSizeSeen = true;
    emit localSizeChanged(size);

    if (!m_isTerm)
        setRemoteSize(size);

    return true;
}

void
TermInstance::setRemoteSize(QSize size)
{
    // qCDebug(lcLayout) << this << "in setRemoteSize, arg" << size;

    if (m_screen->size() != size) {
        emit sizeChanged(size);
        qCDebug(lcLayout) << this << "resize to" << size << "(remote)";
    }

    if (!m_remoteSizeSeen) {
        m_remoteSizeSeen = true;

        if (m_ours) {
            if (m_localSizeSeen && m_localSize != size) {
                m_requestedSize = m_localSize;
                g_listener->pushTermResize(this, m_localSize);
                qCDebug(lcLayout) << this << "requesting size" << m_localSize << "(remote)";
            }
            else if (!m_localSizeSeen) {
                m_requestedSize = size;
            }
        }
    }
}

void
TermInstance::setMousePos(QPoint mousePos)
{
    if (m_mousePos != mousePos) {
        m_mousePos = mousePos;
        emit contentChanged();
    }
}

bool
TermInstance::keyPressEvent(TermManager *manager, QKeyEvent *event, Tsq::TermFlags flags)
{
    QByteArray result = m_keymap->translate(manager, event, flags);
    if (m_overlay) {
        m_overlay->inputEvent(manager, result);
    }
    if (result.isEmpty() || (flags & Tsqt::SelectMode && g_global->inputSelect())) {
        return false;
    }
    g_listener->pushTermInput(this, result);
    return true;
}

void
TermInstance::mouseEvent(Tsq::MouseEventFlags flags, const QPoint &p)
{
    if (!m_overlay) {
        g_listener->pushTermMouseEvent(this, flags, p.x(), p.y());

        if (m_mousePos != p) {
            m_mousePos = p;
            emit contentChanged();
        }
    }
}

void
TermInstance::pushInput(TermManager *manager, const QString &text, bool bracketed)
{
    QByteArray tmp;

    if (m_overlay)
        return;

    if ((m_flags & Tsq::BracketedPasteMode) && bracketed) {
        int idx;
        tmp.append(TSQ_BPM_START, TSQ_BPM_START_LEN);
        tmp.append(text.toUtf8());
        while ((idx = tmp.indexOf(TSQ_BPM_END, TSQ_BPM_START_LEN)) != -1)
            tmp.remove(idx, TSQ_BPM_END_LEN);
        while ((idx = tmp.indexOf(TSQ_BPM_START, TSQ_BPM_START_LEN)) != -1)
            tmp.remove(idx, TSQ_BPM_START_LEN);
        tmp.append(TSQ_BPM_END, TSQ_BPM_END_LEN);
    } else {
        tmp = text.toUtf8();
    }

    if (tmp.size() < PASTE_SIZE_THRESHOLD) {
        g_listener->pushTermInput(this, tmp);
    } else {
        auto *task = new PasteBytesTask(this, tmp);
        task->start(manager);
    }

    auto *scrollport = manager->activeScrollport();
    if (scrollport && scrollport->term() == this)
        emit scrollport->inputReceived();
}

inline void
TermInstance::shutdown()
{
    if (m_server->conn())
        g_listener->pushTermRemove(this);
    else
        m_server->removeTerm(this);
}

void
TermInstance::close(TermManager *manager)
{
    int setting = m_profile->promptClose();
    bool prompt;
    QString message;

    switch (setting) {
    case Tsqt::PromptCloseAlways:
        message = TR_ASK1;
        prompt = true;
        break;
    case Tsqt::PromptCloseProc:
        message = TR_ASK2;
        prompt = m_process.status >= Tsq::TermActive;
        break;
    case Tsqt::PromptCloseSubproc:
        message = TR_ASK3;
        prompt = m_process.status >= Tsq::TermBusy;
        break;
    default:
        prompt = false;
        break;
    }

    if (prompt) {
        auto *box = askBox(TR_TITLE1, message, manager->parent());
        connect(box, &QDialog::finished, [this](int result) {
            if (result == QMessageBox::Yes) {
                shutdown();
            }
        });
        box->show();
    } else {
        shutdown();
    }
}

void
TermInstance::updateStackSpec()
{
    int size = m_stacks.size();
    int val = size ? 65535 : 0;

    for (auto stack: qAsConst(m_stacks))
        if (stack->index() < val)
            val = stack->index();

    if (size > 1)
        val = -val;

    // Note: caller's responsibility to emit stacksChanged

    if (m_stackSpec != val) {
        m_stackSpec = val;
        QString value;

        if (val > 0)
            value = QString::number(val);
        else if (val < 0)
            value = L("%1+").arg(-val);

        const QString &key = g_attr_TSQT_INDEX;
        emit attributeChanged(key, m_attributes[key] = value);
    }
}

void
TermInstance::removeAnnotation(regionid_t id)
{
    const Region *region = m_buffers->safeRegion(id);
    if (region && region->type() == Tsqt::RegionUser)
        g_listener->pushRegionRemove(this, id);
}

void
TermInstance::setSearch(TermSearch &search)
{
    if (m_search != search) {
        m_search = std::move(search);
        emit searchChanged(search.active);
    }
}

static inline void
pullImage(TermInstance *thiz, TermContent *content, TermManager *manager)
{
    if (content->size < IMAGE_SIZE_THRESHOLD) {
        g_listener->pushTermGetImage(thiz, content->id.toULongLong());
    }
    else if (manager) {
        (new FetchImageTask(thiz, content))->start(manager);
    }
}

void
TermInstance::fetchImage(const QString &id, TermManager *manager)
{
    TermContent *content = m_content->content(id);
    if (content && !content->enabled) {
        content->enabled = true;
        content->loaded = false;
        emit m_content->contentFetching(content);
        pullImage(this, content, manager);
    }
}

void
TermInstance::registerImage(const Region *r)
{
    QString id = r->attributes.value(g_attr_CONTENT_ID);
    TermContent *content = m_content->addContent(id, r);

    if (content && content->isinline) {
        // Handle new item
        content->enabled = m_server->serverInfo()->renderImages() >= 0 ?
            m_server->serverInfo()->renderImages() :
            g_global->renderImages();

        if (content->enabled)
            pullImage(this, content, g_listener->activeManager());
        else
            content->loaded = true;
    }
}

void
TermInstance::updateImage(const QString &id, const char *data, size_t len)
{
    TermContent *content = m_content->content(id);

    if (content && content->enabled) {
        content->pixmap.loadFromData((const uchar *)data, len);

        TermMovie *movie = new TermMovie;
        movie->data = QByteArray(data, len);
        movie->buffer.setBuffer(&movie->data);
        movie->movie.setDevice(&movie->buffer);

        if (movie->movie.isValid() && movie->movie.frameCount() > 1)
            content->movie = movie;
        else
            delete movie;

        content->loaded = true;
        emit contentChanged();
    }
}

void
TermInstance::putImage(const Region *r)
{
    m_content->putContent(r->attributes.value(g_attr_CONTENT_ID));
}

const QString &
TermInstance::profileName() const
{
    return m_profile->name();
}

QString
TermInstance::title() const
{
    return m_attributes.value(g_attr_SESSION_TITLE);
}

QString
TermInstance::name() const
{
    QString result = m_format->caption();
    result = result.left(result.indexOf('\n'));
    if (!result.isEmpty())
        return result;
    result = m_attributes.value(g_attr_SESSION_TITLE);
    if (!result.isEmpty())
        return result;
    return m_profile->name();
}

void
TermInstance::setInputFollower(bool inputFollower)
{
    m_inputFollower = inputFollower;
    emit inputChanged();
    reindicate();

    QString key = A(TSQT_ATTR_FOLLOWER);
    if (inputFollower)
        setAttribute(key, C('1'));
    else
        removeAttribute(key);
}

void
TermInstance::setInputLeader(bool inputLeader)
{
    m_inputLeader = inputLeader;
    emit inputChanged();
    reindicate();

    QString key = A(TSQT_ATTR_LEADER);
    if (inputLeader)
        setAttribute(key, C('1'));
    else
        removeAttribute(key);
}
