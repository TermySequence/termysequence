// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "emulator.h"
#include "term.h"
#include "tabstops.h"
#include "palette.h"
#include "locale.h"
#include "config.h"
#include "lib/attrstr.h"

TermEmulator::TermEmulator(TermInstance *parent, Size &size, EmulatorParams &params) :
    m_unicoding(parent->locale()->unicoding()),
    m_parent(parent),
    m_modTimePtr(parent->modTimePtr()),
    m_initflags(params.flags),
    m_flags(m_initflags),
    m_promptNewline(params.promptNewline),
    m_scrollClear(params.scrollClear)
{
    if (size.width() < TERM_MIN_COLS)
        size.setWidth(TERM_MIN_COLS);
    if (size.width() > TERM_MAX_COLS)
        size.setWidth(TERM_MAX_COLS);
    if (size.height() < TERM_MIN_ROWS)
        size.setHeight(TERM_MIN_ROWS);
    if (size.height() > TERM_MAX_ROWS)
        size.setHeight(TERM_MAX_ROWS);

    if (params.caporder == 0)
        params.caporder = 1;
    if (params.caporder > TERM_MAX_CAPORDER)
        params.caporder = TERM_MAX_CAPORDER;

    m_buf[0] = new TermBuffer(this, size.height(), params.caporder, 0);
    m_buf[1] = new TermBuffer(this, size.height(), 0, 1);
    m_tabs = new TermTabStops(size.width());
    m_screen = new TermScreen(this, size);
    m_palette = new TermPalette(params.palette);
}

TermEmulator::TermEmulator(TermInstance *parent, const TermEmulator *copyfrom) :
    m_unicoding(parent->locale()->unicoding()),
    m_parent(parent),
    m_modTimePtr(parent->modTimePtr()),
    m_initflags(copyfrom->m_initflags),
    m_flags(m_initflags),
    m_promptNewline(copyfrom->m_promptNewline),
    m_scrollClear(copyfrom->m_scrollClear)
{
    m_buf[0] = new TermBuffer(this, copyfrom->m_buf[0]);
    m_buf[1] = new TermBuffer(this, copyfrom->m_buf[1]);
    m_tabs = new TermTabStops(*copyfrom->m_tabs);
    m_screen = new TermScreen(this, copyfrom->m_screen);
    m_palette = new TermPalette(*copyfrom->m_palette);
}

TermEmulator::~TermEmulator()
{
    delete m_palette;
    delete m_screen;
    delete m_tabs;
    delete m_buf[1];
    delete m_buf[0];
}

/*
 * Called externally
 */
bool
TermEmulator::termResize(Size &size)
{
    TermInstance::StateLock slock(m_parent, true);
    resetEventState();

    Cursor savedCursor = cursor();

    if (setSize(size))
        reportSizeChanged();
    if (savedCursor != cursor())
        reportCursorChanged();

    return m_stateChanged;
}

bool
TermEmulator::moveMouse(Point &mousePos)
{
    TermInstance::StateLock slock(m_parent, true);

    if (mousePos.x() >= m_screen->width())
        mousePos.setX(m_screen->width() - 1);
    if (mousePos.y() >= m_screen->height())
        mousePos.setY(m_screen->height() - 1);

    if (m_screen->mousePos() == mousePos)
        return false;

    m_screen->setMousePos(mousePos);
    return true;
}

bool
TermEmulator::bufferResize(uint8_t bufid, uint8_t caporder)
{
    bool retval = false;
    bool noScrollback = caporder & 0x80;

    caporder &= 0x7f;
    if (caporder > TERM_MAX_CAPORDER)
        caporder = TERM_MAX_CAPORDER;

    if (bufid == 0 && !noScrollback) {
        TermInstance::StateLock slock(m_parent, true);
        resetEventState();

        retval = m_buf[0]->enableScrollback(caporder);
        m_screen->moveToEnd();
        m_screen->rowAndCursorUpdate();
    }

    return retval;
}

void
TermEmulator::resetEventState()
{
    m_stateChanged = false;
    flagsChanged = false;
    bufferChanged[0][0] = false;
    bufferChanged[0][1] = false;
    bufferChanged[1][0] = false;
    bufferChanged[1][1] = false;
    bufferSwitched = false;
    sizeChanged = false;
    cursorChanged = false;
    bellCount = 0;
    changedAttributes.clear();

    // rows and regions
    m_buf[0]->resetEventState();
    m_buf[1]->resetEventState();
}

std::shared_ptr<const std::string>
TermEmulator::getContent(contentid_t id) const
{
    TermInstance::StateLock slock(m_parent, false);
    const auto i = m_contentMap.find(id);
    return i != m_contentMap.end() ? i->second.data : nullptr;
}

/*
 * Called internally
 */
bool
TermEmulator::setSize(Size &size)
{
    if (size.width() < TERM_MIN_COLS)
        size.setWidth(TERM_MIN_COLS);
    if (size.width() > TERM_MAX_COLS)
        size.setWidth(TERM_MAX_COLS);
    if (size.height() < TERM_MIN_ROWS)
        size.setHeight(TERM_MIN_ROWS);
    if (size.height() > TERM_MAX_ROWS)
        size.setHeight(TERM_MAX_ROWS);

    if (m_screen->size() == size)
        return false;

    int width = size.width(), height = size.height();

    if (m_screen->width() != width) {
        m_tabs->setWidth(width);
        m_screen->setWidth(width);
    }
    if (m_screen->height() != height) {
        unsigned maxChop = m_altActive ? 0 :
            m_screen->bounds().bottom() - m_screen->cursor().y();

        int rc1 = m_buf[0]->setScreenHeight(height, maxChop);
        int rc2 = m_buf[1]->setScreenHeight(height, 0);
        m_screen->setHeight(height, m_altActive ? rc2 : rc1);
    }
    m_parent->resizeFd(size);

    setAttribute(Tsq::attr_SESSION_COLS, std::to_string(width));
    setAttribute(Tsq::attr_SESSION_ROWS, std::to_string(height));
    return true;
}

void
TermEmulator::setWidth(int width)
{
    if (width < TERM_MIN_COLS)
        width = TERM_MIN_COLS;
    if (width > TERM_MAX_COLS)
        width = TERM_MAX_COLS;

    if (m_screen->width() == width)
        return;

    Size size(width, m_screen->height());

    m_tabs->setWidth(width);
    m_screen->setWidth(width);
    m_parent->resizeFd(size);
    reportSizeChanged();

    setAttribute(Tsq::attr_SESSION_COLS, std::to_string(width));
    setAttribute(Tsq::attr_SESSION_ROWS, std::to_string(size.height()));
}

void
TermEmulator::setAltActive(bool altActive)
{
    if (m_altActive != altActive) {
        m_altActive = altActive;
        m_screen->setBuffer(m_buf[altActive]);
    }
}

void
TermEmulator::setAttribute(const std::string &key, const std::string &value)
{
    if (m_parent->setAttribute(key, value)) {
        std::string spec(key);
        spec.push_back('\0');
        spec.append(value);
        spec.push_back('\0');
        changedAttributes[key] = std::move(spec);
    }
}

void
TermEmulator::removeAttribute(const std::string &key)
{
    if (m_parent->removeAttribute(key)) {
        std::string spec(key);
        spec.push_back('\0');
        changedAttributes[key] = std::move(spec);
    }
}

bool
TermEmulator::setFlag(Tsq::TermFlags flag, bool enabled)
{
    TermInstance::StateLock slock(m_parent, true);
    resetEventState();

    Tsq::TermFlags cur = m_flags & flag;
    Tsq::TermFlags next = enabled ? flag : Tsq::None;

    if (!(cur ^ next))
        return false;
    else if (enabled)
        m_flags |= flag;
    else
        m_flags &= ~flag;

    reportFlagsChanged();
    return true;
}

void
TermEmulator::reportAttributeChange(const std::string &key, const std::string &value)
{
    if (key == Tsq::attr_SESSION_PALETTE)
        m_palette->parse(value);
    else if (key == Tsq::attr_PROFILE_PROMPTNEWLINE)
        m_promptNewline = (value == "true");
    else if (key == Tsq::attr_PROFILE_SCROLLCLEAR)
        m_scrollClear = (value == "true");
}

const std::string *
TermEmulator::getContentPtr(contentid_t id) const
{
    const auto i = m_contentMap.find(id);
    return i != m_contentMap.end() ? i->second.data.get() : nullptr;
}

void
TermEmulator::putContent(Region *region)
{
    const char *idstr = region->attributes[Tsq::attr_CONTENT_ID].c_str();
    contentid_t id = strtoull(idstr, NULL, 10);
    auto i = m_contentMap.find(id);

    if (i != m_contentMap.end() && --i->second.refcount == 0)
        m_contentMap.erase(i);
}
