// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "xterm.h"
#include "charsets.h"
#include "base/term.h"
#include "base/tabstops.h"
#include "base/output.h"
#include "base/screeniter.h"
#include "os/logging.h"
#include "lib/attrstr.h"
#include "lib/utf8.h"

using namespace std;

XTermEmulator::XTermEmulator(TermInstance *parent, Size &size, EmulatorParams &params) :
    TermEmulator(parent, size, params),
    m_state(this, parent->translator()),
    m_charset(0, 2, &charset_usascii, &charset_usascii, &charset_latin1supp, &charset_latin1supp),
    m_savedModes{{ 2, true }, { 7, true }, { 1007, true }}
{
    cmdSaveCursor();
}

XTermEmulator::XTermEmulator(TermInstance *parent, const XTermEmulator *copyfrom) :
    TermEmulator(parent, copyfrom),
    m_state(this, parent->translator()),
    m_charset(0, 2, &charset_usascii, &charset_usascii, &charset_latin1supp, &charset_latin1supp),
    m_savedModes{{ 2, true }, { 7, true }, { 1007, true }}
{
    cmdSaveCursor();
}

TermEmulator *
XTermEmulator::duplicate(TermInstance *parent, Size &size)
{
    auto *emulator = new XTermEmulator(parent, this);
    emulator->setSize(size);
    emulator->resetEmulator(Tsq::ResetEmulator);
    emulator->m_screen->cursorMoveX(false, 0, false);
    emulator->eraseInDisplay(0);
    emulator->resetEventState();
    return emulator;
}

bool
XTermEmulator::termReset(const char *buf, unsigned len, Tsq::ResetFlags arg)
{
    utf8::unchecked::iterator<const char *> i(buf), j(buf + len);
    bool isValid = utf8::is_valid(buf, buf + len);

    TermInstance::StateLock slock(m_parent, true);
    resetEventState();
    // save state
    Tsq::TermFlags savedFlags = flags();
    Cursor savedCursor = cursor();

    resetEmulator(arg);

    if (isValid)
        for (; i != j; ++i)
            m_state.process(*i);

    // compare state
    if (savedFlags != flags())
        reportFlagsChanged();
    if (savedCursor != cursor())
        reportCursorChanged();

    return m_stateChanged;
}

void
XTermEmulator::termEventChecked(const char *buf, unsigned len)
{
    string checked;

    try {
        utf8::replace_invalid(buf, buf + len, std::back_inserter(checked));
    }
    catch (const utf8::not_enough_room &) {
        // Note: UTF8CPP provides no obvious way to handle this case
    }

    utf8::unchecked::iterator<string::iterator> i(checked.begin());
    utf8::unchecked::iterator<string::iterator> j(checked.end());

    for (; i != j; ++i)
        m_state.process(*i);
}

bool
XTermEmulator::termEvent(char *buf, unsigned len, bool running, bool chflags)
{
    int off = 0;

    if (running && m_running[0]) {
        memcpy(buf - 8, m_running, 8);
        off = m_running[0];
        m_running[0] = 0;
    }

    utf8::iterator<const char *> i(buf - off, buf - off, buf + len);
    utf8::iterator<const char *> j(buf + len, buf - off, buf + len);

    TermInstance::StateLock slock(m_parent, true);
    resetEventState();
    // save state
    Tsq::TermFlags savedFlags = flags();
    Cursor savedCursor = cursor();

    try {
        for (; i != j; ++i)
            m_state.process(*i);
    }
    catch (const utf8::not_enough_room &) {
        off = len - (i.base() - buf);
        memcpy(m_running + (8 - off), i.base(), off);
        m_running[0] = off;
    }
    catch (const utf8::exception &) {
        termEventChecked(i.base(), len - (i.base() - buf));
    }

    if (m_attributes.flags & Tsq::Command)
        m_screen->handlePartialCommand();

    // compare state
    if (savedFlags != flags() || chflags)
        reportFlagsChanged();
    if (savedCursor != cursor())
        reportCursorChanged();

    return m_stateChanged;
}

bool
XTermEmulator::termSend(const char *buf, unsigned len)
{
    bool lockable = true, query = true;

    utf8::iterator<const char *> i(buf,       buf, buf + len);
    utf8::iterator<const char *> j(buf + len, buf, buf + len);

    try {
        for (; i != j; ++i) {
            unsigned c = *i;

            // check for hard scroll lock
            if ((c == 0x11 || c == 0x13) && lockable) {
                lockable = m_parent->reportHardScrollLock(c == 0x13, query);
                query = false;
            }
        }
    }
    catch (const utf8::exception &) {
        // ignore
    }

    string data(buf, len);
    return m_parent->output()->submitData(std::move(data));
}

void
XTermEmulator::termReply(const char *buf)
{
    unsigned len = strlen(buf);
    string data(buf, len);

    if (!utf8::is_valid(buf, buf + len))
        return;

    if (!(m_flags & Tsq::Controls8Bit))
    {
        utf8::unchecked::iterator<const char *> i(buf), j(buf + len);

        for (; i != j; ++i) {
            unsigned c = *i;

            // replace 8-bit controls with 7-bit controls
            // fortunately, an 8-bit control in UTF-8 is also 2 bytes
            if (c >= 0x80 && c <= 0x9f) {
                data[i.base() - buf] = 0x1b;
                data[i.base() - buf + 1] = c - 0x40;
            }
        }
    }

    m_parent->output()->submitData(std::move(data));
}

void
XTermEmulator::clearScreen()
{
    TermRowIterator iterator(m_screen);

    m_attributes = CellAttributes();
    m_screen->setMargins(m_screen->bounds());

    m_screen->cursorMoveX(false, 0, false);
    m_screen->cursorMoveY(false, 0, false);

    while (!iterator.done) {
        m_screen->resetSingleLine(iterator.y);
        iterator.next();
    }
    m_screen->buffer()->removeRegions(m_screen->offset(), 0);
}

void
XTermEmulator::eraseInDisplay(int type)
{
    TermRowIterator it(m_screen);
    Point p = m_screen->cursor();

    if (m_scrollClear && m_attributes.flags & Tsq::Command &&
        (type == 2 || (type == 0 && p.x() == 0 && p.y() == 0)))
    {
        m_screen->scrollToJob();
    }

    switch (type) {
    case 0:
        while (!it.done) {
            if (it.y == p.y())
                it.singleRow().resize(p.x(), m_unicoding);
            else if (it.y > p.y())
                m_screen->resetSingleLine(it.y);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset() + p.y(), p.x());
        break;
    case 1:
        while (!it.done) {
            if (it.y < p.y())
                m_screen->resetLine(it.y);
            else if (it.y == p.y())
                it.row().erase(0, p.x() + 1, m_unicoding);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset(), 0);
        break;
    case 2:
        while (!it.done) {
            m_screen->resetSingleLine(it.y);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset(), 0);
        break;
    case 3:
        resetEmulator(Tsq::ClearScrollback);
        break;
    }

    m_screen->cursorUpdate();
}

void
XTermEmulator::resetEmulator(Tsq::ResetFlags arg)
{
    Charsets c = {{ &charset_usascii, &charset_usascii, &charset_latin1supp, &charset_latin1supp }};

    if (arg & Tsq::ResetEmulator) {
        m_flags = m_initflags;
        m_screen->reset();
        m_tabs->reset();
        m_buf[1]->clear();
        setAltActive(false);
        m_running[0] = 0;
        m_state.reset();
        m_charset.setCharsets(c, 0, 2, -1);
        m_attributes = CellAttributes();
        cmdSaveCursor();
        m_savedCursor.cursor = Cursor();
        removeAttribute(Tsq::attr_COMMAND);
        removeAttribute(Tsq::attr_CURSOR);
        m_cursorStyle = 1;
        m_titleStack = std::stack<string>();
        m_title2Stack = std::stack<string>();
    }
    if (arg & Tsq::ClearScrollback) {
        m_buf[0]->clearScrollback();
        m_screen->moveToEnd();
        m_screen->rowAndCursorUpdate();
    }
    if (arg & Tsq::ClearScreen) {
        clearScreen();
    }
    if (arg & Tsq::FormFeed) {
        string data(1, '\f');
        m_parent->output()->submitData(std::move(data));
    }
}

void
XTermEmulator::lineFeed()
{
    if (m_screen->cursorAtBottom())
        m_screen->scrollUp();
    else
        m_screen->cursorMoveDown();
}

void
XTermEmulator::carriageReturn()
{
    m_screen->cursorMoveX(false, 0, true);
}

inline void
XTermEmulator::control(const Codepoint c)
{
    // LOGDBG("control: %x\n", c);

    switch (c) {
    case 0x05:
        termReply(m_parent->getAnswerback());
        break;
    case 0x07:
        reportBellRang();
        break;
    case 0x08:
        if (!m_screen->cursorAtLeft()) {
            m_screen->cursorMoveX(true, -1, true);
        }
        else if (m_flags & Tsq::ReverseAutowrap && !m_screen->cursorAtTop()) {
            m_screen->cursorMoveY(true, -1, true);
            m_screen->cursorMoveX(false, m_screen->margins().right(), true);
        }
        break;
    case 0x09:
        m_screen->cursorMoveX(false, m_tabs->getNextTabStop(m_screen->cursor().x()), true);
        break;
    case 0x0a:
    case 0x0b:
    case 0x0c:
        lineFeed();
        if (m_flags & Tsq::NewLine)
            carriageReturn();
        break;
    case 0x0d:
        carriageReturn();
        break;
    case 0x0e:
        m_charset.setLeft(1);
        break;
    case 0x0f:
        m_charset.setLeft(0);
        break;
    case 0x7f:
        /* do nothing */
        break;
    case 0x84:
        lineFeed();
        break;
    case 0x85:
        lineFeed();
        carriageReturn();
        break;
    case 0x88:
        m_tabs->setTabStop(m_screen->cursor().x());
        break;
    case 0x8d:
        if (m_screen->cursorAtTop())
            m_screen->scrollDown();
        else
            m_screen->cursorMoveY(true, -1, true);
        break;
    case 0x8e:
        m_charset.setSingleLeft(2);
        break;
    case 0x8f:
        m_charset.setSingleLeft(3);
        break;
    default:
        m_state.dumpError("", false);
        break;
    }
}

inline void
XTermEmulator::printableCell(const CellAttributes &a, const Codepoint c, int width)
{
    if (m_screen->cursorPastEnd(width) && m_flags & Tsq::Autowrap) {
        m_screen->cursorMoveX(false, 0, true);

        if (m_screen->cursorAtBottom())
            m_screen->scrollUp();
        else
            m_screen->cursorMoveY(true, 1, true);

        if (!(m_flags & Tsq::LeftRightMarginMode))
            m_screen->row().flags = Tsq::Continuation;
    }

    if (m_flags & Tsq::InsertMode)
        m_screen->insertCells(width);

    m_screen->writeCell(a, c, width);
}

inline void
XTermEmulator::printable(const Codepoint c)
{
    CellAttributes a(m_attributes);
    int width;

    if (c < 128) {
        // ASCII fast path
        m_unicoding->restart(c);
        width = 1;
        goto printable;
    }

    width = m_unicoding->widthCategoryOf(c, a.flags);

    if (width > 0) {
    printable:
        printableCell(a, c, width);
    }
    else if (width == 0) {
        m_screen->combineCell(a, c);
    }
    else if (width == -2) {
        // Width upgrade
        if (m_screen->cursorPastEnd(1))
            m_screen->deleteCell();
        else
            m_screen->cursorMoveX(true, -1, true);

        const Codepoint *i, *j;
        m_unicoding->getSeq(i, j);
        printableCell(a, *i, 2);
        while (++i != j)
            m_screen->combineCell(a, *i);
    }
}

void
XTermEmulator::process()
{
    const Codepoint c = m_charset.map(m_state.allSequence().back());

    if (XTermStateMachine::isControlCode(c)) {
        control(c);
    } else {
        printable(c);
    }
}

void
XTermEmulator::internalError(const char *msg)
{
    // Can't use state machine here
    CellAttributes save = m_attributes;
    m_attributes.flags = Tsq::Fg|Tsq::Bg|Tsq::Bold;
    m_attributes.fg = MAKE_COLOR(255, 128, 128);
    m_attributes.bg = MAKE_COLOR(0, 0, 0);

    while (*msg)
        printable(*msg++);

    m_attributes = save;
}

void
XTermEmulator::cmdDisable8BitControls()
{
    m_flags &= ~Tsq::Controls8Bit;
}

void
XTermEmulator::cmdEnable8BitControls()
{
    m_flags |= Tsq::Controls8Bit;
}

void
XTermEmulator::cmdDECDoubleHeightTop()
{
    if (!(m_flags & Tsq::LeftRightMarginMode))
        m_screen->setLineFlags(Tsq::DblWidthLine|Tsq::DblTopLine);
}

void
XTermEmulator::cmdDECDoubleHeightBottom()
{
    if (!(m_flags & Tsq::LeftRightMarginMode))
        m_screen->setLineFlags(Tsq::DblWidthLine|Tsq::DblBottomLine);
}

void
XTermEmulator::cmdDECSingleWidth()
{
    m_screen->setLineFlags(Tsq::NoLineFlags);
}

void
XTermEmulator::cmdDECDoubleWidth()
{
    if (!(m_flags & Tsq::LeftRightMarginMode))
        m_screen->setLineFlags(Tsq::DblWidthLine);
}

void
XTermEmulator::cmdDECScreenAlignmentTest()
{
    TermRowIterator iterator(m_screen);
    int n = m_screen->width();

    clearScreen();

    while (!iterator.done) {
        CellRow &row = iterator.row();

        while (row.columns() < n)
            row.append(m_attributes, 'E', 1);

        iterator.next();
    }
}

void
XTermEmulator::cmdDesignateCharset94()
{
    int slot;

    switch (m_state.allSequence().at(1)) {
    case '(':
        slot = 0;
        break;
    case ')':
        slot = 1;
        break;
    case '*':
        slot = 2;
        break;
    default:
        slot = 3;
        break;
    }

    switch (m_state.allSequence().at(2)) {
    case 'B':
    case '1':
        m_charset.setCharset(slot, &charset_usascii);
        break;
    case '0':
    case '2':
        m_charset.setCharset(slot, &charset_declinedrawing);
        break;
    case 'A':
        m_charset.setCharset(slot, &charset_britishnrc);
        break;
    }
}

void
XTermEmulator::cmdDesignateCharset96()
{
    int slot;

    switch (m_state.allSequence().at(1)) {
    case '-':
        slot = 1;
        break;
    case '.':
        slot = 2;
        break;
    default:
        slot = 3;
        break;
    }

    switch (m_state.allSequence().at(2)) {
    case 'A':
        m_charset.setCharset(slot, &charset_latin1supp);
        break;
    }
}

void
XTermEmulator::cmdSaveCursor()
{
    m_savedCursor.flags = m_flags & (Tsq::OriginMode|Tsq::Autowrap);
    m_savedCursor.cursor = m_screen->cursor();
    m_savedCursor.attributes = m_attributes;
    m_savedCursor.gl = m_charset.left();
    m_savedCursor.gr = m_charset.right();
    m_savedCursor.nextgl = m_charset.nextLeft();
    m_savedCursor.charsets = m_charset.charsets();
}

void
XTermEmulator::cmdRestoreCursor()
{
    m_flags &= ~(Tsq::OriginMode|Tsq::Autowrap);
    m_flags |= m_savedCursor.flags;
    m_screen->setStayWithinMargins(m_flags & Tsq::OriginMode);

    m_screen->cursorMoveX(false, m_savedCursor.cursor.x(), false);
    m_screen->cursorMoveY(false, m_savedCursor.cursor.y(), false);
    m_screen->setCursorPastEnd(m_savedCursor.cursor.pastEnd());
    m_attributes = m_savedCursor.attributes;

    m_charset.setCharsets(m_savedCursor.charsets, m_savedCursor.gl, m_savedCursor.gr, m_savedCursor.nextgl);
}

void
XTermEmulator::cmdApplicationKeypad()
{
    m_flags |= Tsq::AppKeyPad;
}

void
XTermEmulator::cmdNormalKeypad()
{
    m_flags &= ~Tsq::AppKeyPad;
}

void
XTermEmulator::cmdResetEmulator()
{
    resetEmulator(Tsq::ResetEmulator|Tsq::ClearScreen);
}

void
XTermEmulator::cmdInvokeCharset()
{
    switch (m_state.allSequence().at(1)) {
    case 'n':
        m_charset.setLeft(2);
        break;
    case 'o':
        m_charset.setLeft(3);
        break;
    case '|':
        m_charset.setRight(3);
        break;
    case '}':
        m_charset.setRight(2);
        break;
    case '~':
        m_charset.setRight(1);
        break;
    }
}

void
XTermEmulator::cmdInsertCharacters()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->insertCells(times);
}

void
XTermEmulator::cmdCursorUp()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveY(true, -times, true);
}

void
XTermEmulator::cmdCursorDown()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveY(true, times, true);
}

void
XTermEmulator::cmdCursorForward()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveX(true, times, true);
}

void
XTermEmulator::cmdCursorBackward()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveX(true, -times, true);
}

void
XTermEmulator::cmdCursorNextLine()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveY(true, times, true);
    m_screen->cursorMoveX(false, 0, true);
}

void
XTermEmulator::cmdCursorPreviousLine()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    m_screen->cursorMoveY(true, -times, true);
    m_screen->cursorMoveX(false, 0, true);
}

void
XTermEmulator::cmdCursorHorizontalAbsolute()
{
    int col = m_state.var(0).toUInt();

    if (col != 0)
        --col;

    m_screen->cursorMoveX(false, col, true);
}

void
XTermEmulator::cmdCursorPosition()
{
    auto vars = m_state.varList(0);
    int row = (vars.size() > 0) ? vars[0]->toUInt() : 0;
    int col = (vars.size() > 1) ? vars[1]->toUInt() : 0;

    if (row != 0)
        --row;
    if (col != 0)
        --col;

    m_screen->cursorMoveY(false, row, false);
    m_screen->cursorMoveX(false, col, false);
}

void
XTermEmulator::cmdTabForward()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->cursorMoveX(false, m_tabs->getNextTabStop(m_screen->cursor().x()), true);
}

void
XTermEmulator::cmdEraseInDisplay()
{
    eraseInDisplay(m_state.var(0).toUInt());
}

void
XTermEmulator::cmdSelectiveEraseInDisplay()
{
    TermRowIterator it(m_screen);
    Point p = m_screen->cursor();

    // XXX clear line attributes on full line erase?

    switch (m_state.var(0).toUInt()) {
    case 0:
        while (!it.done) {
            if (it.y == p.y())
                it.row().selectiveErase(p.x(), m_unicoding);
            if (it.y > p.y())
                it.row().selectiveErase(0, m_unicoding);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset() + p.y(), p.x());
        break;
    case 1:
        while (!it.done) {
            if (it.y < p.y())
                it.row().selectiveErase(0, m_unicoding);
            if (it.y == p.y())
                it.row().selectiveErase(0, p.x() + 1, m_unicoding);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset(), 0);
        break;
    case 2:
        while (!it.done) {
            it.row().selectiveErase(0, m_unicoding);
            it.next();
        }
        m_screen->buffer()->removeRegions(m_screen->offset(), 0);
        break;
    }

    m_screen->cursorUpdate();
}

void
XTermEmulator::cmdEraseInLine()
{
    CellRow &row = m_screen->row();
    bool pastEnd = m_screen->cursor().pastEnd();
    int x = m_screen->cursor().x() + pastEnd;

    switch (m_state.var(0).toUInt()) {
    case 0:
        row.resize(x, m_unicoding);
        break;
    case 1:
        row.erase(0, x + 1, m_unicoding);
        break;
    case 2:
        row.erase();
        break;
    }

    m_screen->cursorUpdate();
    if (pastEnd)
        m_screen->setCursorPastEnd(true);
}

void
XTermEmulator::cmdSelectiveEraseInLine()
{
    CellRow &row = m_screen->row();
    bool pastEnd = m_screen->cursor().pastEnd();
    int x = m_screen->cursor().x() + pastEnd;

    switch (m_state.var(0).toUInt()) {
    case 0:
        row.selectiveErase(x, row.columns(), m_unicoding);
        break;
    case 1:
        row.selectiveErase(0, x + 1, m_unicoding);
        break;
    case 2:
        row.selectiveErase(0, row.columns(), m_unicoding);
        break;
    }

    m_screen->cursorUpdate();
    if (pastEnd)
        m_screen->setCursorPastEnd(true);
}

void
XTermEmulator::cmdInsertLines()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->insertRow();
}

void
XTermEmulator::cmdDeleteLines()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->deleteRow();
}

void
XTermEmulator::cmdDeleteCharacters()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->deleteCell();
}

void
XTermEmulator::cmdScrollUp()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->scrollUp();
}

void
XTermEmulator::cmdScrollDown()
{
    if (m_state.varCount(0) > 1) {
        // Highlight mouse tracking (unsupported)
        return;
    }

    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->scrollDown();
}

void
XTermEmulator::cmdResetTitleModes()
{
    for (const Codestring *var: m_state.varList(0)) {
        switch (var->toUInt()) {
        case 0:
            m_flags &= ~Tsq::TitleModeSetHex;
            break;
        case 1:
            m_flags &= ~Tsq::TitleModeQueryHex;
            break;
        }
    }
}

void
XTermEmulator::cmdSetTitleModes()
{
    for (const Codestring *var: m_state.varList(0)) {
        switch (var->toUInt()) {
        case 0:
            m_flags |= Tsq::TitleModeSetHex;
            break;
        case 1:
            m_flags |= Tsq::TitleModeQueryHex;
            break;
        }
    }
}

void
XTermEmulator::cmdEraseCharacters()
{
    int times = m_state.var(0).toUInt();
    CellRow &row = m_screen->row();
    int x = m_screen->cursor().x();

    if (times == 0)
        times = 1;

    row.erase(x, x + times, m_unicoding);
}

void
XTermEmulator::cmdTabBackward()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    while (times--)
        m_screen->cursorMoveX(false, m_tabs->getPrevTabStop(m_screen->cursor().x()), true);
}

void
XTermEmulator::cmdRepeatCharacter()
{
    int times = m_state.var(0).toUInt();

    if (times == 0)
        times = 1;

    unsigned pos = m_screen->cursor().pos();
    if (pos) {
        pos -= !m_screen->cursor().pastEnd();
        Codestring cs(m_screen->row().substr(pos, pos + 1, m_unicoding));

        while (times--)
            for (Codepoint c: cs)
                printable(c);
    }
}

void
XTermEmulator::cmdCursorVerticalAbsolute()
{
    int row = m_state.var(0).toUInt();

    if (row != 0)
        --row;

    m_screen->cursorMoveY(false, row, true);
}

void
XTermEmulator::cmdTabClear()
{
    switch (m_state.var(0).toUInt()) {
    case 0:
        m_tabs->clearTabStop(m_screen->cursor().x());
        break;
    case 3:
        m_tabs->clearTabStops();
        break;
    }
}

void
XTermEmulator::cmdSetMode()
{
    for (const Codestring *var: m_state.varList(0)) {
        switch (var->toUInt()) {
        case 2:
            m_flags |= Tsq::KeyboardLock;
            continue;
        case 4:
            m_flags |= Tsq::InsertMode;
            continue;
        case 12:
            m_flags |= Tsq::SendReceive;
            continue;
        case 20:
            m_flags |= Tsq::NewLine;
            continue;
        }
    }
}

void
XTermEmulator::setPrivateMode(int mode)
{
    switch (mode) {
    case 1:
        m_flags |= Tsq::AppCuKeys;
        break;
    case 2:
        m_flags |= Tsq::Ansi;
        for (int slot = 0; slot < 4; ++slot)
            m_charset.setCharset(slot, &charset_usascii);
        break;
    case 3:
        if (m_flags & Tsq::AllowColumnChange)
            setWidth(132);
        clearScreen();
        m_flags &= ~Tsq::LeftRightMarginMode;
        break;
    case 4:
        m_flags |= Tsq::SmoothScrolling;
        break;
    case 5:
        m_flags |= Tsq::ReverseVideo;
        break;
    case 6:
        m_flags |= Tsq::OriginMode;
        m_screen->setStayWithinMargins(true);
        break;
    case 7:
        m_flags |= Tsq::Autowrap;
        break;
    case 8:
        m_flags |= Tsq::Autorepeat;
        break;
    case 9:
        m_flags &= ~Tsq::MouseModeMask;
        m_flags |= Tsq::X10MouseMode;
        break;
    case 12:
        if (!(m_cursorStyle & 1))
            setAttribute(Tsq::attr_CURSOR, std::to_string(--m_cursorStyle));
        break;
    case 25:
        m_flags |= Tsq::CursorVisible;
        break;
    case 40:
        m_flags |= Tsq::AllowColumnChange;
        break;
    case 45:
        m_flags |= Tsq::ReverseAutowrap;
        break;
    case 47:
        setAltActive(true);
        m_flags |= Tsq::AppScreen;
        break;
    case 66:
        cmdApplicationKeypad();
        break;
    case 69: {
        m_flags |= Tsq::LeftRightMarginMode;
        TermRowIterator iterator(m_screen);
        while (!iterator.done) {
            m_screen->setLineFlags(iterator.y, Tsq::NoLineFlags);
            iterator.next();
        }
        break;
    }
    case 1000:
        m_flags &= ~Tsq::MouseModeMask;
        m_flags |= Tsq::NormalMouseMode;
        break;
    case 1001:
        m_flags &= ~Tsq::MouseModeMask;
        m_flags |= Tsq::HighlightMouseMode;
        break;
    case 1002:
        m_flags &= ~Tsq::MouseModeMask;
        m_flags |= Tsq::ButtonEventMouseMode;
        break;
    case 1003:
        m_flags &= ~Tsq::MouseModeMask;
        m_flags |= Tsq::AnyEventMouseMode;
        break;
    case 1004:
        m_flags |= Tsq::FocusEventMode;
        break;
    case 1005:
        m_flags &= ~Tsq::ExtMouseModeMask;
        m_flags |= Tsq::Utf8ExtMouseMode;
        break;
    case 1006:
        m_flags &= ~Tsq::ExtMouseModeMask;
        m_flags |= Tsq::SgrExtMouseMode;
        break;
    case 1007:
        m_flags |= Tsq::AltScrollMouseMode;
        break;
    case 1015:
        m_flags &= ~Tsq::ExtMouseModeMask;
        m_flags |= Tsq::UrxvtExtMouseMode;
        break;
    case 1047:
        setAltActive(true);
        m_flags |= Tsq::AppScreen;
        break;
    case 1048:
        cmdSaveCursor();
        break;
    case 1049:
        cmdSaveCursor();
        m_buf[1]->clear();
        setAltActive(true);
        m_flags |= Tsq::AppScreen;
        break;
    case 2004:
        m_flags |= Tsq::BracketedPasteMode;
        break;
    }
}

void
XTermEmulator::cmdDECPrivateModeSet()
{
    for (const Codestring *var: m_state.varList(0))
        setPrivateMode(var->toUInt());
}

void
XTermEmulator::cmdDECPrivateModeSave()
{
    for (const Codestring *var: m_state.varList(0)) {
        int arg = var->toUInt();
        switch (arg) {
        case 1:
            m_savedModes[arg] = m_flags & Tsq::AppCuKeys;
            continue;
        case 2:
            m_savedModes[arg] = m_flags & Tsq::Ansi;
            continue;
        case 4:
            m_savedModes[arg] = m_flags & Tsq::SmoothScrolling;
            continue;
        case 5:
            m_savedModes[arg] = m_flags & Tsq::ReverseVideo;
            continue;
        case 6:
            m_savedModes[arg] = m_flags & Tsq::OriginMode;
            continue;
        case 7:
            m_savedModes[arg] = m_flags & Tsq::Autowrap;
            continue;
        case 8:
            m_savedModes[arg] = m_flags & Tsq::Autorepeat;
            continue;
        case 9:
            m_savedModes[arg] = m_flags & Tsq::X10MouseMode;
            continue;
        case 12:
            m_savedModes[arg] = m_cursorStyle & 1;
            continue;
        case 25:
            m_savedModes[arg] = m_flags & Tsq::CursorVisible;
            continue;
        case 40:
            m_savedModes[arg] = m_flags & Tsq::AllowColumnChange;
            continue;
        case 45:
            m_savedModes[arg] = m_flags & Tsq::ReverseAutowrap;
            continue;
        case 47:
        case 1047:
        case 1049:
            m_savedModes[arg] = m_flags & Tsq::AppScreen;
            continue;
        case 66:
            m_savedModes[arg] = m_flags & Tsq::AppKeyPad;
            continue;
        case 69:
            m_savedModes[arg] = m_flags & Tsq::LeftRightMarginMode;
            continue;
        case 1000:
            m_savedModes[arg] = m_flags & Tsq::NormalMouseMode;
            continue;
        case 1001:
            m_savedModes[arg] = m_flags & Tsq::HighlightMouseMode;
            continue;
        case 1002:
            m_savedModes[arg] = m_flags & Tsq::ButtonEventMouseMode;
            continue;
        case 1003:
            m_savedModes[arg] = m_flags & Tsq::AnyEventMouseMode;
            continue;
        case 1004:
            m_savedModes[arg] = m_flags & Tsq::FocusEventMode;
            continue;
        case 1005:
            m_savedModes[arg] = m_flags & Tsq::Utf8ExtMouseMode;
            continue;
        case 1006:
            m_savedModes[arg] = m_flags & Tsq::SgrExtMouseMode;
            continue;
        case 1007:
            m_savedModes[arg] = m_flags & Tsq::AltScrollMouseMode;
            continue;
        case 1015:
            m_savedModes[arg] = m_flags & Tsq::UrxvtExtMouseMode;
            continue;
        case 2004:
            m_savedModes[arg] = m_flags & Tsq::BracketedPasteMode;
            continue;
        }
    }
}

void
XTermEmulator::cmdResetMode()
{
    for (const Codestring *var: m_state.varList(0)) {
        switch (var->toUInt()) {
        case 2:
            m_flags &= ~Tsq::KeyboardLock;
            continue;
        case 4:
            m_flags &= ~Tsq::InsertMode;
            continue;
        case 12:
            m_flags &= ~Tsq::SendReceive;
            continue;
        case 20:
            m_flags &= ~Tsq::NewLine;
            continue;
        }
    }
}

void
XTermEmulator::resetPrivateMode(int mode)
{
    switch (mode) {
    case 1:
        m_flags &= ~Tsq::AppCuKeys;
        break;
    case 2:
        m_flags &= ~Tsq::Ansi;
        break;
    case 3:
        if (m_flags & Tsq::AllowColumnChange)
            setWidth(80);
        clearScreen();
        m_flags &= ~Tsq::LeftRightMarginMode;
        break;
    case 4:
        m_flags &= ~Tsq::SmoothScrolling;
        break;
    case 5:
        m_flags &= ~Tsq::ReverseVideo;
        break;
    case 6:
        m_flags &= ~Tsq::OriginMode;
        m_screen->setStayWithinMargins(false);
        break;
    case 7:
        m_flags &= ~Tsq::Autowrap;
        break;
    case 8:
        m_flags &= ~Tsq::Autorepeat;
        break;
    case 9:
    case 1000:
    case 1001:
    case 1002:
    case 1003:
        m_flags &= ~Tsq::MouseModeMask;
        break;
    case 12:
        if (m_cursorStyle & 1)
            setAttribute(Tsq::attr_CURSOR, std::to_string(++m_cursorStyle));
        break;
    case 25:
        m_flags &= ~Tsq::CursorVisible;
        break;
    case 40:
        m_flags &= ~Tsq::AllowColumnChange;
        break;
    case 45:
        m_flags &= ~Tsq::ReverseAutowrap;
        break;
    case 47:
        setAltActive(false);
        m_flags &= ~Tsq::AppScreen;
        break;
    case 66:
        cmdNormalKeypad();
        break;
    case 69: {
        m_flags &= ~Tsq::LeftRightMarginMode;
        Rect margins = m_screen->margins();
        margins.setLeft(0);
        margins.setWidth(m_screen->width());
        m_screen->setMargins(margins);
        break;
    }
    case 1004:
        m_flags &= ~Tsq::FocusEventMode;
        break;
    case 1005:
    case 1006:
    case 1015:
        m_flags &= ~Tsq::ExtMouseModeMask;
        break;
    case 1007:
        m_flags &= ~Tsq::AltScrollMouseMode;
        break;
    case 1047:
        if (m_altActive)
            m_buf[1]->clear();
        setAltActive(false);
        m_flags &= ~Tsq::AppScreen;
        break;
    case 1048:
        cmdRestoreCursor();
        break;
    case 1049:
        setAltActive(false);
        m_flags &= ~Tsq::AppScreen;
        cmdRestoreCursor();
        break;
    case 2004:
        m_flags &= ~Tsq::BracketedPasteMode;
        break;
    }
}

void
XTermEmulator::cmdDECPrivateModeReset()
{
    for (const Codestring *var: m_state.varList(0))
        resetPrivateMode(var->toUInt());
}

void
XTermEmulator::cmdDECPrivateModeRestore()
{
    for (const Codestring *var: m_state.varList(0)) {
        int arg = var->toUInt();
        auto j = m_savedModes.find(arg);
        if (j != m_savedModes.end() && j->second)
            setPrivateMode(arg);
        else
            resetPrivateMode(arg);
    }
}

void
XTermEmulator::cmdCharacterAttributes()
{
    auto vars = m_state.varList(0);
    uint8_t r, g, b;

    if (vars.size() == 0) {
        m_attributes.flags &= ~Tsq::All;
        m_attributes.fg = m_attributes.bg = 0;
    }
    for (int i = 0, n = vars.size(); i < n; ++i)
    {
        int arg = vars[i]->toUInt();
        switch (arg) {
        case 0:
            m_attributes.flags &= ~Tsq::All;
            m_attributes.fg = m_attributes.bg = 0;
            continue;
        case 1:
            m_attributes.flags |= Tsq::Bold;
            continue;
        case 2:
            m_attributes.flags |= Tsq::Faint;
            continue;
        case 3:
            m_attributes.flags |= Tsq::Italics;
            continue;
        case 4:
            m_attributes.flags |= Tsq::Underline;
            continue;
        case 5:
            m_attributes.flags |= Tsq::Blink;
            m_flags |= Tsq::BlinkSeen;
            continue;
        case 6:
            m_attributes.flags |= Tsq::FastBlink;
            m_flags |= Tsq::BlinkSeen;
            continue;
        case 7:
            m_attributes.flags |= Tsq::Inverse;
            continue;
        case 8:
            m_attributes.flags |= Tsq::Invisible;
            continue;
        case 9:
            m_attributes.flags |= Tsq::Strikethrough;
            continue;
        case 10:
            m_attributes.flags &= ~Tsq::FontMask;
            continue;
        case 11:
            m_attributes.flags |= Tsq::AltFont1;
            continue;
        case 12:
            m_attributes.flags |= Tsq::AltFont2;
            continue;
        case 20:
            m_attributes.flags |= Tsq::AltFont0;
            continue;
        case 21:
            m_attributes.flags |= Tsq::DblUnderline;
            continue;
        case 22:
            m_attributes.flags &= ~(Tsq::Bold|Tsq::Faint);
            continue;
        case 23:
            m_attributes.flags &= ~(Tsq::Italics|Tsq::AltFont0);
            continue;
        case 24:
            m_attributes.flags &= ~(Tsq::Underline|Tsq::DblUnderline);
            continue;
        case 25:
            m_attributes.flags &= ~(Tsq::Blink|Tsq::FastBlink);
            continue;
        case 27:
            m_attributes.flags &= ~Tsq::Inverse;
            continue;
        case 28:
            m_attributes.flags &= ~Tsq::Invisible;
            continue;
        case 29:
            m_attributes.flags &= ~Tsq::Strikethrough;
            continue;
        case 39:
            m_attributes.flags &= ~(Tsq::Fg|Tsq::FgIndex);
            m_attributes.fg = 0;
            continue;
        case 49:
            m_attributes.flags &= ~(Tsq::Bg|Tsq::BgIndex);
            m_attributes.bg = 0;
            continue;
        case 51:
            m_attributes.flags |= Tsq::Framed;
            continue;
        case 52:
            m_attributes.flags |= Tsq::Encircled;
            continue;
        case 53:
            m_attributes.flags |= Tsq::Overline;
            continue;
        case 54:
            m_attributes.flags &= ~(Tsq::Framed|Tsq::Encircled);
            continue;
        case 55:
            m_attributes.flags &= ~Tsq::Overline;
            continue;
        }

        if (arg == 38 || arg == 48) {
            if (++i == n)
                break;
            switch (vars[i]->toUInt()) {
            case 5:
                if (++i == n)
                    return;
                if (arg == 38) {
                    m_attributes.flags |= Tsq::Fg|Tsq::FgIndex;
                    m_attributes.fg = vars[i]->toUInt();
                } else {
                    m_attributes.flags |= Tsq::Bg|Tsq::BgIndex;
                    m_attributes.bg = vars[i]->toUInt();
                }
                break;
            case 2:
                if (n - i < 4)
                    return;
                r = vars[++i]->toUInt();
                g = vars[++i]->toUInt();
                b = vars[++i]->toUInt();
                if (arg == 38) {
                    m_attributes.flags |= Tsq::Fg;
                    m_attributes.flags &= ~Tsq::FgIndex;
                    m_attributes.fg = MAKE_COLOR(r, g, b);
                } else {
                    m_attributes.flags |= Tsq::Bg;
                    m_attributes.flags &= ~Tsq::BgIndex;
                    m_attributes.bg = MAKE_COLOR(r, g, b);
                }
                break;
            default:
                return;
            }
        }
        else if (arg >= 30 && arg <= 37) {
            m_attributes.flags |= Tsq::Fg|Tsq::FgIndex;
            m_attributes.fg = arg - 30;
        }
        else if (arg >= 40 && arg <= 47) {
            m_attributes.flags |= Tsq::Bg|Tsq::BgIndex;
            m_attributes.bg = arg - 40;
        }
        else if (arg >= 90 && arg <= 97) {
            m_attributes.flags |= Tsq::Fg|Tsq::FgIndex;
            m_attributes.fg = arg - 82;
        }
        else if (arg >= 100 && arg <= 107) {
            m_attributes.flags |= Tsq::Bg|Tsq::BgIndex;
            m_attributes.bg = arg - 92;
        }
    }
}

void
XTermEmulator::cmdProtectionAttribute()
{
    switch (m_state.var(0).toUInt()) {
    case 0:
    case 2:
        m_attributes.flags &= ~Tsq::Protected;
        break;
    case 1:
        m_attributes.flags |= Tsq::Protected;
        break;
    }
}

void
XTermEmulator::cmdSetCursorStyle()
{
    unsigned arg = m_state.var(0).toUInt();
    if (arg == 0)
        arg = 1;
    if (arg <= 6)
        setAttribute(Tsq::attr_CURSOR, std::to_string(m_cursorStyle = arg));
}

void
XTermEmulator::cmdSetTopBottomMargins()
{
    int h = m_screen->height();
    auto vars = m_state.varList(0);
    int top = (vars.size() > 0) ? vars[0]->toUInt() : 1;
    int bot = (vars.size() > 1) ? vars[1]->toUInt() : h;

    if (top == 0)
        top = 1;
    if (bot == 0)
        bot = h;

    if (top > h || bot > h || top >= bot)
        return;

    Rect margins = m_screen->margins();
    margins.setTop(top - 1);
    margins.setBottom(bot - 1);
    m_screen->setMargins(margins);

    m_screen->cursorMoveX(false, 0, false);
    m_screen->cursorMoveY(false, 0, false);
}

void
XTermEmulator::cmdSetLeftRightMargins()
{
    auto vars = m_state.varList(0);

    if (!(m_flags & Tsq::LeftRightMarginMode) && vars.empty()) {
        cmdSaveCursor();
        return;
    }

    int w = m_screen->width();
    int left = (vars.size() > 0) ? vars[0]->toUInt() : 1;
    int right = (vars.size() > 1) ? vars[1]->toUInt() : w;

    if (left == 0)
        left = 1;
    if (right == 0)
        right = w;

    if (left > w || right > w || left >= right)
        return;

    Rect margins = m_screen->margins();
    margins.setLeft(left - 1);
    margins.setRight(right - 1);
    m_screen->setMargins(margins);

    m_screen->cursorMoveX(false, 0, false);
    m_screen->cursorMoveY(false, 0, false);
}
