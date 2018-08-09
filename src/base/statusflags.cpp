// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "statusflags.h"
#include "statusarrange.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "scrollport.h"
#include "settings/global.h"

#define TR_TEXT1 TL("window-text", "Terminal owner: %1@%2")
#define TR_TEXT2 TL("window-text", \
    "Warning: Soft Scroll Lock enabled. Use Scroll Lock to disable")
#define TR_TEXT3 TL("window-text", "Warning: Hard Scroll Lock enabled. Use Ctrl+Q to disable")
#define TR_TEXT4 TL("window-text", \
    "Environment out of date. Run %1 check-env for details")
#define TR_PROP1 T3("TermPropModel", "Command Mode", "property-name")
#define TR_PROP2 T3("TermPropModel", "Selection Mode", "property-name")
#define TR_PROP3 T3("TermPropModel", "Hard Scroll Lock", "property-name")
#define TR_PROP4 T3("TermPropModel", "Soft Scroll Lock", "property-name")
#define TR_PROP5 T3("TermPropModel", "Send Focus Events", "property-name")
#define TR_PROP6 T3("TermPropModel", "Send Mouse Events", "property-name")
#define TR_PROP7 T3("TermPropModel", "Bracketed Paste Mode", "property-name")
#define TR_PROP8 T3("TermPropModel", "Origin Mode", "property-name")
#define TR_PROP9 T3("TermPropModel", "Left-Right Margin Mode", "property-name")
#define TR_PROP10 T3("TermPropModel", "Application Cursor Keys", "property-name")
#define TR_PROP11 T3("TermPropModel", "Application Keypad", "property-name")
#define TR_PROP12 T3("TermPropModel", "Alternate Buffer Active", "property-name")
#define TR_PROP13 T3("TermPropModel", "Insert Mode", "property-name")
#define TR_PROP14 T3("TermPropModel", "Reverse Video", "property-name")
#define TR_PROP15 TL("TermPropModel", "Raw Mode", "property-name")
#define TR_PROP16 TL("TermPropModel", "Scrolling with Terminal Owner", "property-name")
#define TR_PROP17 TL("TermPropModel", "Autoscroll Disabled", "property-name")
#define TR_PROP18 TL("TermPropModel", "Data Transfer Throttled", "property-name")
#define TR_PROP19 TL("TermPropModel", "Input Multiplexing Leader", "property-name")
#define TR_PROP20 TL("TermPropModel", "Input Multiplexing Follower", "property-name")
#define TR_PROP21 T3("TermPropModel", "Terminal Updates Throttled", "property-name")
#define TR_PROP22 TL("TermPropModel", "Environment Out of Date", "property-name")

#define STRING_FLAGS    0
#define STRING_PROC     1
#define STRING_INPUT    2
#define STRING_LOCK     3
#define STRING_THROTTLE 4
#define STRING_ENV      5
#define N_STRINGS       6

struct FlagTip
{
    const char *str;
    const char *comment;
};
struct FlagDef
{
    Tsq::TermFlags flag;
    const QLatin1String text;
    FlagTip tip;
};

static const FlagDef s_defs[] = {
    { Tsqt::CommandMode, A("<font style='background-color:@minorbg@;color:@minorfg@'>COM</font>"), TR_PROP1 },
    { Tsqt::SelectMode, A("<font style='background-color:@minorbg@;color:@minorfg@'>SEL</font>"), TR_PROP2 },
    { Tsq::HardScrollLock, A("<font style='font-weight:bold;background-color:@majorbg@;color:@majorfg@'>HSL</font>"), TR_PROP3 },
    { Tsq::SoftScrollLock, A("<font style='font-weight:bold;background-color:@minorbg@;color:@minorfg@'>SSL</font>"), TR_PROP4 },
    { Tsq::RateLimited, A("RATELIM"), TR_PROP21 },
    { Tsq::FocusEventMode, A("FOC"), TR_PROP5 },
    { Tsq::MouseModeMask, A("MOU"), TR_PROP6 },
    { Tsq::BracketedPasteMode, A("BPM"), TR_PROP7 },
    { Tsq::OriginMode, A("ORM"), TR_PROP8 },
    { Tsq::LeftRightMarginMode, A("LRM"), TR_PROP9 },
    { Tsq::AppCuKeys, A("CUR"), TR_PROP10 },
    { Tsq::AppKeyPad, A("PAD"), TR_PROP11 },
    { Tsq::AppScreen, A("SCR"), TR_PROP12 },
    { Tsq::InsertMode, A("INS"), TR_PROP13 },
    { Tsq::ReverseVideo, A("REV"), TR_PROP14 },
    { 0, A("") }
};

StatusFlags::StatusFlags(TermManager *manager, MainStatus *parent) :
    m_parent(parent),
    m_manager(manager),
    m_strings(N_STRINGS),
    m_followspec(L("<font style='background-color:@minorbg@;color:@minorfg@'>FOLLOW</font>")),
    m_leaderspec(L("<font style='background-color:@minorbg@;color:@minorfg@'>IML</font>")),
    m_throttlespec(L("<font style='background-color:@majorbg@;color:@majorfg@'>WAIT</font>")),
    m_envspec(L("<font style='background-color:@majorbg@;color:@majorfg@'>ENV</font>"))
{
    m_rawTip = TR_PROP15 + A(" (ISIG = 0)");
    m_followTip = TR_PROP16;
    m_holdTip = TR_PROP17;
    m_envTip = TR_PROP22;

    connect(manager,
            SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));
    m_mocFlags = connect(g_listener, &TermListener::flagsChanged,
                         this, &StatusFlags::handleFlagsChanged);
    connect(this, SIGNAL(clicked()), SLOT(handleClicked()));
}

void
StatusFlags::doPolish()
{
    for (const FlagDef *ptr = s_defs; ptr->flag; ++ptr) {
        FlagString f = {
            ptr->flag, ptr->text,
            TL("TermPropModel", ptr->tip.str, ptr->tip.comment)
        };

        f.str.replace(A("@majorbg@"), g_global->color(MajorBg).name());
        f.str.replace(A("@majorfg@"), g_global->color(MajorFg).name());
        f.str.replace(A("@minorbg@"), g_global->color(MinorBg).name());
        f.str.replace(A("@minorfg@"), g_global->color(MinorFg).name());

        m_flagspecs.append(f);
        m_mask |= ptr->flag;
    }

    m_followspec.replace(A("@minorbg@"), g_global->color(MinorBg).name());
    m_followspec.replace(A("@minorfg@"), g_global->color(MinorFg).name());
    m_leaderspec.replace(A("@minorbg@"), g_global->color(MinorBg).name());
    m_leaderspec.replace(A("@minorfg@"), g_global->color(MinorFg).name());
    m_throttlespec.replace(A("@majorbg@"), g_global->color(MajorBg).name());
    m_throttlespec.replace(A("@majorfg@"), g_global->color(MajorFg).name());
    m_envspec.replace(A("@majorbg@"), g_global->color(MajorBg).name());
    m_envspec.replace(A("@majorfg@"), g_global->color(MajorFg).name());

    handleFlagsChanged(g_listener->flags());
}

void
StatusFlags::handleClicked()
{
    if (m_term)
        m_manager->actionViewTerminalInfo(m_term->idStr());
}

void
StatusFlags::handleAttributes()
{
    const auto &attr = m_term->attributes();

    if (m_term->ours() || !m_term->isTerm()) {
        m_parent->clearStatus(STATUSPRIO_OWNERSHIP);
    } else {
        QString u = attr.value(g_attr_OWNER_USER, g_str_unknown);
        QString h = attr.value(g_attr_OWNER_HOST, g_str_unknown);

        m_parent->showStatus(TR_TEXT1.arg(u, h), STATUSPRIO_OWNERSHIP);
    }

    QString str, tip;

    if (m_term->attributes().value(g_attr_ENV_DIRTY) != A("1")) {
        m_parent->clearStatus(STATUSPRIO_ENVIRON);
    } else {
        m_parent->showStatus(TR_TEXT4.arg(ABBREV_NAME "ctl"), STATUSPRIO_ENVIRON);

        str = m_envspec;
        tip = str + A(": ") + m_envTip;
    }

    updateText(STRING_ENV, str, tip);
}

void
StatusFlags::handleAttributeChanged(const QString &key, const QString &value)
{
    if (key.startsWith(g_attr_OWNER_PREFIX) || key == g_attr_ENV_DIRTY)
        handleAttributes();
}

void
StatusFlags::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    disconnect(m_mocFlags);

    if (m_term) {
        m_term->disconnect(this);
        m_scrollport->disconnect(this);
    }

    if ((m_term = term)) {
        connect(m_term, SIGNAL(attributeChanged(QString,QString)), SLOT(handleAttributeChanged(const QString&,const QString&)));
        connect(m_term, SIGNAL(processChanged(const QString&)), SLOT(handleProcessChanged(const QString&)));
        connect(m_term, SIGNAL(inputChanged()), SLOT(handleInputChanged()));
        connect(m_term, SIGNAL(throttleChanged(bool)), SLOT(handleThrottleChanged(bool)));

        m_scrollport = scrollport;
        m_mocFlags = connect(m_scrollport, SIGNAL(flagsChanged(Tsq::TermFlags)), SLOT(handleFlagsChanged(Tsq::TermFlags)));
        connect(m_scrollport, SIGNAL(lockedChanged(bool)), SLOT(handleLockedChanged()));
        connect(m_scrollport, SIGNAL(followingChanged(bool)), SLOT(handleLockedChanged()));

        handleFlagsChanged(m_scrollport->flags());
        handleProcessChanged(g_attr_PROC_TERMIOS);
        handleInputChanged();
        handleAttributes();
        handleLockedChanged();
        handleThrottleChanged(m_term->throttled());
    } else {
        m_strings.resize(1);
        m_strings.resize(N_STRINGS);
        m_mocFlags = connect(g_listener, &TermListener::flagsChanged, this, &StatusFlags::handleFlagsChanged);
        handleFlagsChanged(g_listener->flags());
    }
}

void
StatusFlags::updateText(int index, const QString &str, const QString &tip)
{
    auto &elt = m_strings[index];
    if (elt.first != str) {
        elt.first = str;
        elt.second = tip;

        QStringList strings, tooltips;

        for (const auto &elt: qAsConst(m_strings))
            if (!elt.first.isEmpty()) {
                strings.append(elt.first);
                tooltips.append(A("<nobr>") + elt.second + A("</nobr>"));
            }

        setText(strings.join(' '));
        setToolTip(tooltips.join(A("<br>")));
    }
}

void
StatusFlags::handleFlagsChanged(Tsq::TermFlags flags)
{
    flags &= m_mask;
    if (m_flags == flags)
        return;

    QStringList strings, tooltips;
    bool wasLocked = m_flags & (Tsq::SoftScrollLock|Tsq::HardScrollLock);

    m_flags = flags;

    for (auto &i: qAsConst(m_flagspecs))
        if (flags & i.flag) {
            strings.append(i.str);
            tooltips.append(i.str + A(": ") + i.tooltip);
        }

    updateText(STRING_FLAGS, strings.join(' '), tooltips.join(A("<br>")));

    if (flags & Tsq::SoftScrollLock) {
        m_parent->showMinor(TR_TEXT2);
    } else if (flags & Tsq::HardScrollLock) {
        m_parent->showMinor(TR_TEXT3);
    } else if (wasLocked) {
        m_parent->clearMinor();
    }
}

void
StatusFlags::handleProcessChanged(const QString &key)
{
    if (key == g_attr_PROC_TERMIOS) {
        QString str, tip;
        const auto *process = m_term->process();

        if (process->termiosRaw) {
            str = A("RAW");
            tip = str + A(": ") + m_rawTip;
        }

        updateText(STRING_PROC, str, tip);
    }
}

void
StatusFlags::handleLockedChanged()
{
    QString str, tip;
    bool locked = m_scrollport->locked();
    bool following = m_scrollport->following();

    if (following) {
        str = m_followspec;
        tip = str + A(": ") + m_followTip;
    }
    else if (locked) {
        str = A("HOLD");
        tip = str + A(": ") + m_holdTip;
    }

    updateText(STRING_LOCK, str, tip);
}

void
StatusFlags::handleThrottleChanged(bool throttled)
{
    QString str, tip;

    if (throttled) {
        str = m_throttlespec;
        tip = str + A(": ") + TR_PROP18;
    }

    updateText(STRING_THROTTLE, str, tip);
}

void
StatusFlags::handleInputChanged()
{
    QString str, tip;

    if (m_term) {
        if (m_term->inputLeader()) {
            str = m_leaderspec;
            tip = str + A(": ") + TR_PROP19;
        } else if (m_term->inputFollower()) {
            str = A("IMF");
            tip = str + A(": ") + TR_PROP20;
        }
    }

    updateText(STRING_INPUT, str, tip);
}
