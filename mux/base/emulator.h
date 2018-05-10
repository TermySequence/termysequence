// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "lib/flags.h"
#include "lib/unicode.h"
#include "screen.h"
#include "attributemap.h"

#include <memory>

class TermBuffer;
class TermInstance;
class TermTabStops;
class TermPalette;
class Translator;

struct EmulatorParams {
    Tsq::Unicoding *unicoding;
    const Translator *translator;
    Tsq::TermFlags flags;
    std::string palette;
    uint8_t caporder;
    bool promptNewline;
    bool scrollClear;
    unsigned fileLimit;
};

class TermEmulator
{
protected:
    Tsq::Unicoding *m_unicoding;
    TermScreen *m_screen;
    TermBuffer *m_buf[2];
    TermInstance *m_parent;
    TermTabStops *m_tabs;
    TermPalette *m_palette;

    const int32_t *m_modTimePtr;
    const Tsq::TermFlags m_initflags;
    Tsq::TermFlags m_flags;
    bool m_altActive = false;
    bool m_promptNewline;
    bool m_scrollClear;

    struct ContentRec {
        std::shared_ptr<const std::string> data;
        unsigned refcount;
        inline ContentRec(std::string *str) : data(str), refcount(1) {}
    };
    std::unordered_map<contentid_t,ContentRec> m_contentMap;

    // locked
    bool setSize(Size &size);
    void setWidth(int width);
    void setAltActive(bool altActive);
    void addContent(contentid_t id, std::string &content);

    // unlocked
    virtual void termReply(const char *buf) = 0;

    // event state
    void resetEventState();
    bool m_stateChanged = false;

public:
    // event state
    bool flagsChanged = false;
    bool bufferChanged[2][2]{};
    bool bufferSwitched = false;
    bool sizeChanged = false;
    bool cursorChanged = false;
    uint32_t bellCount = 0;
    AttributeMap changedAttributes;

protected:
    TermEmulator(TermInstance *parent, const TermEmulator *copyfrom);
public:
    TermEmulator(TermInstance *parent, Size &size, EmulatorParams &params);
    virtual ~TermEmulator();
    virtual TermEmulator* duplicate(TermInstance *parent, Size &size) = 0;

    // locked
    inline Tsq::Unicoding* unicoding() const { return m_unicoding; }
    inline TermBuffer* buffer(unsigned i) { return m_buf[i]; }
    inline const int32_t* modTimePtr() const { return m_modTimePtr; }
    inline const Rect& margins() const { return m_screen->margins(); }
    inline const Cursor& cursor() const { return m_screen->cursor(); }
    inline const Point& mousePos() const { return m_screen->mousePos(); }
    inline const Size& size() const { return m_screen->size(); }
    inline bool altActive() const { return m_altActive; }
    inline Tsq::TermFlags flags() const { return m_flags; }
    inline const Region* safeRegion(bufreg_t i) const
    { return m_buf[BUFREG_BUF(i)]->safeRegion(BUFREG_REG(i)); }

    // locked
    inline void reportFlagsChanged() { flagsChanged = m_stateChanged = true; }
    inline void reportCursorChanged() { cursorChanged = m_stateChanged = true; }
    inline void reportBufferCapacity(uint8_t id)
    { bufferChanged[id][0] = bufferChanged[id][1] = m_stateChanged = true; }
    inline void reportBufferLength(uint8_t id)
    { bufferChanged[id][0] = m_stateChanged = true; }
    inline void reportBufferSwitched() { bufferSwitched = m_stateChanged = true; }
    inline void reportSizeChanged() { sizeChanged = m_stateChanged = true; }
    inline void reportBellRang() { ++bellCount; m_stateChanged = true; }

    // locked
    void setAttribute(const std::string &key, const std::string &value);
    void removeAttribute(const std::string &key);
    void reportAttributeChange(const std::string &key, const std::string &value);
    const std::string* getContentPtr(contentid_t id) const;
    void putContent(Region *region);

    // unlocked
    std::shared_ptr<const std::string> getContent(contentid_t id) const;
    bool termResize(Size &size);
    bool bufferResize(uint8_t bufid, uint8_t caporder);
    bool moveMouse(Point &mousePos);
    bool setFlag(Tsq::TermFlags flag, bool enabled);
    virtual bool termEvent(char *buf, unsigned len, bool running, bool chflags) = 0;
    virtual bool termReset(const char *buf, unsigned len, Tsq::ResetFlags arg) = 0;
    virtual bool termSend(const char *buf, unsigned len) = 0;
    virtual bool termMouse(unsigned event, unsigned x, unsigned y) = 0;
};

inline void
TermScreen::cursorUpdate(const CellRow &row)
{
    row.updateCursor(m_cursor, m_emulator->unicoding());
}

inline void
TermScreen::cursorUpdate()
{
    constRow().updateCursor(m_cursor, m_emulator->unicoding());
}

inline CellRow &
TermScreen::rowUpdate()
{
    index_t i = m_offset + m_cursor.y();
    m_row = &m_buffer->rawRow(i);

    if (i == m_buffer->size() - 1 ||
        !((m_nextRow = &m_buffer->rawRow(i + 1))->flags & Tsq::Continuation))
    {
        m_nextRow = nullptr;
    }

    return *m_row;
}

inline void
TermScreen::rowAndCursorUpdate()
{
    cursorUpdate(rowUpdate());
}
