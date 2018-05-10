// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/emulator.h"
#include "base/cell.h"
#include "machine.h"
#include "charset.h"
#include "savedcursor.h"

#include <stack>

class XTermEmulator final: public TermEmulator
{
private:
    char m_running[8] = {};

    XTermStateMachine m_state;

    CellAttributes m_attributes;
    XTermSavedCursor m_savedCursor;
    TermCharset m_charset;
    std::unordered_map<int,bool> m_savedModes;

    std::string m_sivars[3];
    unsigned m_cursorStyle = 1;

    std::stack<std::string> m_titleStack, m_title2Stack;

    void termReply(const char *buf);
    void termEventChecked(const char *buf, unsigned len);

    void clearScreen();
    void eraseInDisplay(int type);
    void resetEmulator(Tsq::ResetFlags arg);

    void lineFeed();
    void carriageReturn();
    void control(const Codepoint c);
    void printableCell(const CellAttributes &a, const Codepoint c, int width);
    void printable(const Codepoint c);

    void setPrivateMode(int mode);
    void resetPrivateMode(int mode);

public:
    bool termEvent(char *buf, unsigned len, bool running, bool chflags);
    bool termReset(const char *buf, unsigned len, Tsq::ResetFlags arg);
    bool termSend(const char *buf, unsigned len);
    bool termMouse(unsigned event, unsigned x, unsigned y);
    void internalError(const char *msg);

    void process();

    void cmdDisable8BitControls();
    void cmdEnable8BitControls();
    void cmdDECDoubleHeightTop();
    void cmdDECDoubleHeightBottom();
    void cmdDECSingleWidth();
    void cmdDECDoubleWidth();
    void cmdDECScreenAlignmentTest();
    void cmdDesignateCharset94();
    void cmdDesignateCharset96();
    void cmdSaveCursor();
    void cmdRestoreCursor();
    void cmdApplicationKeypad();
    void cmdNormalKeypad();
    void cmdResetEmulator();
    void cmdInvokeCharset();

    void cmdInsertCharacters();
    void cmdCursorUp();
    void cmdCursorDown();
    void cmdCursorForward();
    void cmdCursorBackward();
    void cmdCursorNextLine();
    void cmdCursorPreviousLine();
    void cmdCursorHorizontalAbsolute();
    void cmdCursorPosition();
    void cmdTabForward();
    void cmdEraseInDisplay();
    void cmdSelectiveEraseInDisplay();
    void cmdEraseInLine();
    void cmdSelectiveEraseInLine();
    void cmdInsertLines();
    void cmdDeleteLines();
    void cmdDeleteCharacters();
    void cmdScrollUp();
    void cmdScrollDown();
    void cmdResetTitleModes();
    void cmdSetTitleModes();
    void cmdEraseCharacters();
    void cmdTabBackward();
    void cmdRepeatCharacter();
    void cmdSendDeviceAttributes();
    void cmdSendDeviceAttributes2();
    void cmdCursorVerticalAbsolute();
    void cmdTabClear();
    void cmdSetMode();
    void cmdDECPrivateModeSet();
    void cmdDECPrivateModeSave();
    void cmdResetMode();
    void cmdDECPrivateModeReset();
    void cmdDECPrivateModeRestore();
    void cmdModeRequest();
    void cmdDECPrivateModeRequest();
    void cmdCharacterAttributes();
    void cmdSetModifierResources();
    void cmdDeviceStatusReport();
    void cmdDisableModifierResources();
    void cmdSetCursorStyle();
    void cmdProtectionAttribute();
    void cmdSetTopBottomMargins();
    void cmdSetLeftRightMargins();
    void cmdWindowOps();
    void cmdIgnored();

    void dcsRequestStatusString();
    void oscMain();
    void osc0(std::string &str, int arg);
    void osc3(std::string str);
    void osc4(const std::string &str, int offset, int max);
    void osc7(const std::string &str);
    void osc8(std::string &str, const char *type);
    void osc10(const std::string &str, int start);
    void osc52(const std::string &str);
    void osc104(const std::string &str, int offset, int max);
    void osc110(int num);
    void osc133();
    void osc513(std::string &str);
    void osc514(std::string &str);
    void osc1337(std::string &str);
    void osc1337File(std::string &str);

private:
    XTermEmulator(TermInstance *parent, const XTermEmulator *copyfrom);
public:
    XTermEmulator(TermInstance *parent, Size &size, EmulatorParams &params);
    TermEmulator* duplicate(TermInstance *parent, Size &size);
};
