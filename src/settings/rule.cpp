// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/keys.h"
#include "rule.h"

#include <QRegularExpression>
#include <cstdio>
#include <cctype>
#include <utf8.h>

// XXX there is a valgrind issue with these regexps
static const QRegularExpression s_commandCond(L("[-+]Command$"));
static const QRegularExpression s_selectCond(L("[-+]Selection$"));
static const QRegularExpression s_shiftCond(L("[-+]Shift$"));
static const QRegularExpression s_controlCond(L("[-+]Control$"));
static const QRegularExpression s_altCond(L("[-+]Alt$"));
static const QRegularExpression s_metaCond(L("[-+]Meta$"));
static const QRegularExpression s_anymodCond(L("[-+]AnyMod$"));
static const QRegularExpression s_keypadCond(L("[-+]KeyPad$"));
static const QRegularExpression s_ansiCond(L("[-+]Ansi$"));
static const QRegularExpression s_newlineCond(L("[-+]NewLine$"));
static const QRegularExpression s_appcukeysCond(L("[-+]AppCuKeys$"));
static const QRegularExpression s_appscreenCond(L("[-+]AppScreen$"));
static const QRegularExpression s_appkeypadCond(L("[-+]AppKeyPad$"));

static const QRegularExpression s_space(L("\\s"));
static const QRegularExpression s_hexcode(L("\\\\x([0-9A-Fa-f][0-9A-Fa-f])"));
static const QRegularExpression s_unicode(L("\\\\u([0-9A-Fa-f]{2,5})"));

//
// Plain rule
//
QByteArray
KeymapRule::parseBytes(QString spec)
{
    QByteArray bytes = spec.toUtf8();
    bytes.replace(B("\\E"), B("\x1b"));
    bytes.replace(B("\\a"), B("\x07"));
    bytes.replace(B("\\b"), B("\x08"));
    bytes.replace(B("\\t"), B("\x09"));
    bytes.replace(B("\\n"), B("\x0a"));
    bytes.replace(B("\\v"), B("\x0b"));
    bytes.replace(B("\\f"), B("\x0c"));
    bytes.replace(B("\\r"), B("\x0d"));

    QRegularExpressionMatch match;
    while ((match = s_hexcode.match(spec)).hasMatch())
    {
        unsigned char uc = match.captured(1).toInt(NULL, 16);
        char *c = reinterpret_cast<char*>(&uc);

        bytes.replace(match.captured(), QByteArray::fromRawData(c, 1));
        spec.remove(match.captured());
    }
    while ((match = s_unicode.match(spec)).hasMatch())
    {
        char buf[8], *end;
        int code[1] = { match.captured(1).toInt(NULL, 16) };

        end = utf8::utf32to8(code, code + 1, buf);

        bytes.replace(match.captured(), QByteArray::fromRawData(buf, end - buf));
        spec.remove(match.captured());
    }

    // Replace invalid UTF-8
    QString tmp = bytes;
    return tmp.toUtf8();
}

static QByteArray
writeBytes(const QByteArray &bytes)
{
    QByteArray result(1, '"');

    const char *begin = bytes.data(), *end = bytes.data() + bytes.size();
    utf8::iterator<const char*> i(begin, begin, end), j(end, begin, end);

    while (i != j) {
        unsigned code = *i;
        // XXX support remaining C99 escape codes and e
        // XXX provide a way to specify a literal asterisk
        switch (code) {
        case '\x1b': result.append(B("\\E")); break;
        case '\x07': result.append(B("\\a")); break;
        case '\x08': result.append(B("\\b")); break;
        case '\x09': result.append(B("\\t")); break;
        case '\x0a': result.append(B("\\n")); break;
        case '\x0b': result.append(B("\\v")); break;
        case '\x0c': result.append(B("\\f")); break;
        case '\x0d': result.append(B("\\r")); break;
        default:
            if (code > 255 || isprint(code)) {
                begin = i.base();
                end = (++i).base();
                result.append(begin, end - begin);
                continue;
            } else {
                char buf[16];
                snprintf(buf, sizeof(buf), "\\x%02x", code);
                result.append(buf);
                break;
            }
        }
        ++i;
    }

    return result.append('"');
}

void
KeymapRule::write(QByteArray &line) const
{
    line.append(continuesCombo ? "digraph" : "key");
    line.append(' ');
    line.append(keyName);
    line.append(' ');

    if (mask & Tsqt::CommandMode)
        line.append(conditions & Tsqt::CommandMode ? "+Command" : "-Command");
    if (mask & Tsqt::SelectMode)
        line.append(conditions & Tsqt::SelectMode ? "+Selection" : "-Selection");
    if (mask & Tsqt::Shift)
        line.append(conditions & Tsqt::Shift ? "+Shift" : "-Shift");
    if (mask & Tsqt::Control)
        line.append(conditions & Tsqt::Control ? "+Control" : "-Control");
    if (mask & Tsqt::Alt)
        line.append(conditions & Tsqt::Alt ? "+Alt" : "-Alt");
    if (mask & Tsqt::Meta)
        line.append(conditions & Tsqt::Meta ? "+Meta" : "-Meta");
    if (mask & Tsqt::AnyMod)
        line.append(conditions & Tsqt::AnyMod ? "+AnyMod" : "-AnyMod");
    if (mask & Tsqt::KeyPad)
        line.append(conditions & Tsqt::KeyPad ? "+KeyPad" : "-KeyPad");
    if (mask & Tsq::AppScreen)
        line.append(conditions & Tsq::AppScreen ? "+AppScreen" : "-AppScreen");
    if (mask & Tsq::AppCuKeys)
        line.append(conditions & Tsq::AppCuKeys ? "+AppCuKeys" : "-AppCuKeys");
    if (mask & Tsq::AppKeyPad)
        line.append(conditions & Tsq::AppKeyPad ? "+AppKeyPad" : "-AppKeyPad");
    if (mask & Tsq::NewLine)
        line.append(conditions & Tsq::NewLine ? "+NewLine" : "-NewLine");
    if (mask & Tsq::Ansi)
        line.append(conditions & Tsq::Ansi ? "+Ansi" : "-Ansi");

    line.append(" : ");

    if (line.size() < 32)
        line.append(QByteArray(32 - line.size(), ' '));
    else if (line.size() < 56)
        line.append(QByteArray(56 - line.size(), ' '));

    line.append(outcomeStr);
    line.append('\n');
}

void
KeymapRule::parse(const QStringList &parts, bool special_)
{
    QString condstr = parts[2];
    QString bytes = parts[3];
    QRegularExpressionMatch match;

    conditions = 0;
    mask = 0;
    startsCombo = special_ && bytes == "BeginDigraph";
    continuesCombo = (parts[1] == A("digraph"));
    special = special_;
    outcome = special_ ? bytes.toUtf8() : parseBytes(bytes);

    condstr.remove(s_space);

    for (;;) {
        if ((match = s_commandCond.match(condstr, -8)).hasMatch()) {
            mask |= Tsqt::CommandMode;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::CommandMode : 0;
            condstr.chop(8);
        }
        else if ((match = s_selectCond.match(condstr, -10)).hasMatch()) {
            mask |= Tsqt::SelectMode;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::SelectMode : 0;
            condstr.chop(10);
        }
        else if ((match = s_shiftCond.match(condstr, -6)).hasMatch()) {
            mask |= Tsqt::Shift;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::Shift : 0;
            condstr.chop(6);
        }
        else if ((match = s_controlCond.match(condstr, -8)).hasMatch()) {
            mask |= Tsqt::Control;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::Control : 0;
            condstr.chop(8);
        }
        else if ((match = s_altCond.match(condstr, -4)).hasMatch()) {
            mask |= Tsqt::Alt;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::Alt : 0;
            condstr.chop(4);
        }
        else if ((match = s_metaCond.match(condstr, -5)).hasMatch()) {
            mask |= Tsqt::Meta;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::Meta : 0;
            condstr.chop(5);
        }
        else if ((match = s_anymodCond.match(condstr, -7)).hasMatch()) {
            mask |= Tsqt::AnyMod;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::AnyMod : 0;
            condstr.chop(7);
        }
        else if ((match = s_keypadCond.match(condstr, -7)).hasMatch()) {
            mask |= Tsqt::KeyPad;
            conditions |= (match.captured().at(0) == '+') ? Tsqt::KeyPad : 0;
            condstr.chop(7);
        }
        else if ((match = s_ansiCond.match(condstr, -5)).hasMatch()) {
            mask |= Tsq::Ansi;
            conditions |= (match.captured().at(0) == '+') ? Tsq::Ansi : 0;
            condstr.chop(5);
        }
        else if ((match = s_newlineCond.match(condstr, -8)).hasMatch()) {
            mask |= Tsq::NewLine;
            conditions |= (match.captured().at(0) == '+') ? Tsq::NewLine : 0;
            condstr.chop(8);
        }
        else if ((match = s_appcukeysCond.match(condstr, -10)).hasMatch()) {
            mask |= Tsq::AppCuKeys;
            conditions |= (match.captured().at(0) == '+') ? Tsq::AppCuKeys : 0;
            condstr.chop(10);
        }
        else if ((match = s_appscreenCond.match(condstr, -10)).hasMatch()) {
            mask |= Tsq::AppScreen;
            conditions |= (match.captured().at(0) == '+') ? Tsq::AppScreen : 0;
            condstr.chop(10);
        }
        else if ((match = s_appkeypadCond.match(condstr, -10)).hasMatch()) {
            mask |= Tsq::AppKeyPad;
            conditions |= (match.captured().at(0) == '+') ? Tsq::AppKeyPad : 0;
            condstr.chop(10);
        }
        else {
            keyName = condstr;
            break;
        }
    }
}

void
KeymapRule::populateStrings()
{
    outcomeStr = special ? outcome : writeBytes(outcome);

    modifierStr.clear();
    if (mask & Tsqt::KeyPad) {
        modifierStr.append(conditions & Tsqt::KeyPad ? '+' : '-');
        modifierStr.append(A("Keypad"));
    }
    if (mask & Tsqt::Meta) {
        modifierStr.append(conditions & Tsqt::Meta ? '+' : '-');
        modifierStr.append(A("Meta"));
    }
    if (mask & Tsqt::Control) {
        modifierStr.append(conditions & Tsqt::Control ? '+' : '-');
        modifierStr.append(A("Ctrl"));
    }
    if (mask & Tsqt::Alt) {
        modifierStr.append(conditions & Tsqt::Alt ? '+' : '-');
        modifierStr.append(A("Alt"));
    }
    if (mask & Tsqt::Shift) {
        modifierStr.append(conditions & Tsqt::Shift ? '+' : '-');
        modifierStr.append(A("Shift"));
    }
    if (mask & Tsqt::AnyMod) {
        modifierStr.append(conditions & Tsqt::AnyMod ? '+' : '-');
        modifierStr.append(A("AnyMod"));
    }

    additionalStr.clear();
    if (mask & Tsqt::CommandMode) {
        additionalStr.append(conditions & Tsqt::CommandMode ? '+' : '-');
        additionalStr.append(A("Command"));
    }
    if (mask & Tsqt::SelectMode) {
        additionalStr.append(conditions & Tsqt::SelectMode ? '+' : '-');
        additionalStr.append(A("Selection"));
    }
    if (mask & Tsq::AppScreen) {
        additionalStr.append(conditions & Tsq::AppScreen ? '+' : '-');
        additionalStr.append(A("AppScreen"));
    }
    if (mask & Tsq::AppCuKeys) {
        additionalStr.append(conditions & Tsq::AppCuKeys ? '+' : '-');
        additionalStr.append(A("AppCursorKeys"));
    }
    if (mask & Tsq::AppKeyPad) {
        additionalStr.append(conditions & Tsq::AppKeyPad ? '+' : '-');
        additionalStr.append(A("AppKeypad"));
    }
    if (mask & Tsq::NewLine) {
        additionalStr.append(conditions & Tsq::NewLine ? '+' : '-');
        additionalStr.append(A("Newline"));
    }
    if (mask & Tsq::Ansi) {
        additionalStr.append(conditions & Tsq::Ansi ? '+' : '-');
        additionalStr.append(A("Ansi"));
    }

    expressionStr = keyName;
    if (conditions & Tsqt::AnyMod)
        expressionStr.prepend(A("AnyMod+"));
    if (conditions & Tsqt::Shift)
        expressionStr.prepend(A("Shift+"));
    if (conditions & Tsqt::Alt)
        expressionStr.prepend(A("Alt+"));
    if (conditions & Tsqt::Control)
        expressionStr.prepend(A("Ctrl+"));
    if (conditions & Tsqt::Meta)
        expressionStr.prepend(A("Meta+"));
    if (conditions & Tsqt::KeyPad)
        expressionStr.prepend(A("Keypad+"));
}

const char *
KeymapRule::getFlagName(int flag)
{
    switch (flag) {
    case RULEFLAG_SHIFT:
        return TN("KeymapFlagsModel", "Shift");
    case RULEFLAG_CONTROL:
        return TN("KeymapFlagsModel", "Control");
    case RULEFLAG_ALT:
        return TN("KeymapFlagsModel", "Alt");
    case RULEFLAG_META:
        return TN("KeymapFlagsModel", "Meta");
    case RULEFLAG_ANYMOD:
        return TN("KeymapFlagsModel", "AnyMod");
    case RULEFLAG_KEYPAD:
        return TN("KeymapFlagsModel", "KeyPad");
    case RULEFLAG_COMMAND:
        return TN("KeymapFlagsModel", "Command");
    case RULEFLAG_SELECT:
        return TN("KeymapFlagsModel", "Selection");
    case RULEFLAG_APPSCREEN:
        return TN("KeymapFlagsModel", "AppScreen");
    case RULEFLAG_APPCUKEYS:
        return TN("KeymapFlagsModel", "AppCursorKeys");
    case RULEFLAG_APPKEYPAD:
        return TN("KeymapFlagsModel", "AppKeyPad");
    case RULEFLAG_NEWLINE:
        return TN("KeymapFlagsModel", "Newline");
    case RULEFLAG_ANSI:
        return TN("KeymapFlagsModel", "Ansi");
    default:
        return TN("KeymapFlagsModel", "Unknown");
    }
}

const char *
KeymapRule::getFlagDescription(int flag)
{
    switch (flag) {
    case RULEFLAG_SHIFT:
        return TN("KeymapFlagsModel", "A Shift key on the keyboard is pressed");
    case RULEFLAG_CONTROL:
        return TN("KeymapFlagsModel", "A Ctrl key on the keyboard is pressed");
    case RULEFLAG_ALT:
        return TN("KeymapFlagsModel", "An Alt key on the keyboard is pressed");
    case RULEFLAG_META:
        return TN("KeymapFlagsModel", "A Meta key on the keyboard is pressed");
    case RULEFLAG_ANYMOD:
        return TN("KeymapFlagsModel", "Any modifier key on the keyboard is pressed");
    case RULEFLAG_KEYPAD:
        return TN("KeymapFlagsModel", "A keypad button is pressed");
    case RULEFLAG_COMMAND:
        return TN("KeymapFlagsModel", "Application is in command mode");
    case RULEFLAG_SELECT:
        return TN("KeymapFlagsModel", "A text selection is active in the viewport");
    case RULEFLAG_APPSCREEN:
        return TN("KeymapFlagsModel", "Terminal Alternate Screen Buffer is active");
    case RULEFLAG_APPCUKEYS:
        return TN("KeymapFlagsModel", "Terminal is in Application Cursor Keys mode");
    case RULEFLAG_APPKEYPAD:
        return TN("KeymapFlagsModel", "Terminal is in Application Keypad mode");
    case RULEFLAG_NEWLINE:
        return TN("KeymapFlagsModel", "Terminal is in Newline mode");
    case RULEFLAG_ANSI:
        return TN("KeymapFlagsModel", "Terminal is in Ansi mode (VT100 and above)");
    default:
        return TN("KeymapFlagsModel", "Unknown");
    }
}

Tsq::TermFlags
KeymapRule::getFlagValue(int flag)
{
    switch (flag) {
    case RULEFLAG_SHIFT:
        return Tsqt::Shift;
    case RULEFLAG_CONTROL:
        return Tsqt::Control;
    case RULEFLAG_ALT:
        return Tsqt::Alt;
    case RULEFLAG_META:
        return Tsqt::Meta;
    case RULEFLAG_ANYMOD:
        return Tsqt::AnyMod;
    case RULEFLAG_KEYPAD:
        return Tsqt::KeyPad;
    case RULEFLAG_COMMAND:
        return Tsqt::CommandMode;
    case RULEFLAG_SELECT:
        return Tsqt::SelectMode;
    case RULEFLAG_APPSCREEN:
        return Tsq::AppScreen;
    case RULEFLAG_APPCUKEYS:
        return Tsq::AppCuKeys;
    case RULEFLAG_APPKEYPAD:
        return Tsq::AppKeyPad;
    case RULEFLAG_NEWLINE:
        return Tsq::NewLine;
    case RULEFLAG_ANSI:
        return Tsq::Ansi;
    default:
        return 0;
    }
}

//
// List-based rule
//
OrderedKeymapRule::OrderedKeymapRule(const KeymapRule &r_, int i_, int p_) :
    KeymapRule(r_),
    index(i_),
    parentIndex(p_)
{
    int childIndex = 0;
    int key = 0;
    int priority = 0;

    for (const auto &i: comboRuleset) {
        auto *rule = new OrderedKeymapRule(*i.second, childIndex++, index);

        if (key != rule->key) {
            key = rule->key;
            priority = 0;
        }
        rule->priority = ++priority;
        comboRulelist.append(rule);
    }

    key = 0;
    for (auto i = comboRulelist.rbegin(), j = comboRulelist.rend(); i != j; ++i) {
        if (key != (*i)->key) {
            key = (*i)->key;
            priority = (*i)->priority;
        }
        (*i)->maxPriority = priority;
    }

    comboRuleset.clear();
}

OrderedKeymapRule::OrderedKeymapRule()
{
    conditions = 0;
    mask = 0;
    key = NEW_RULE_KEY;
    startsCombo = false;
    continuesCombo = false;
    special = true;
    keyName = NEW_RULE_KEY_NAME;
    priority = 1;

    populateStrings();
}

//
// Set-based rule container
//
KeymapRuleset::KeymapRuleset(const KeymapRuleset &other)
{
    for (auto &i: qAsConst(other))
        m_set.emplace(i.first, new KeymapRule(*i.second));
}

KeymapRuleset::~KeymapRuleset()
{
    for (auto &&i: m_set)
        delete i.second;
}

void
KeymapRuleset::insert(KeymapRule *rule)
{
    m_set.emplace(rule->key, rule);
}

void
KeymapRuleset::clear()
{
    for (auto &&i: m_set)
        delete i.second;

    m_set.clear();
}

KeymapRuleset &
KeymapRuleset::operator=(const KeymapRuleset &other)
{
    clear();

    for (auto &&i: other)
        m_set.emplace(i.first, new KeymapRule(*i.second));

    return *this;
}

//
// List-based rule container
//
KeymapRulelist::KeymapRulelist(const KeymapRulelist &other)
{
    for (auto &&i: other)
        m_list.push_back(new OrderedKeymapRule(*i));
}

KeymapRulelist::~KeymapRulelist()
{
    for (auto i: m_list)
        delete i;
}

void
KeymapRulelist::erase(size_t pos)
{
    auto i = m_list.begin() + pos;
    delete *i;
    m_list.erase(i);
}

OrderedKeymapRule *
KeymapRulelist::take(size_t pos)
{
    auto i = m_list.begin() + pos;
    auto *ptr = *i;
    m_list.erase(i);
    return ptr;
}

void
KeymapRulelist::insert(size_t pos, OrderedKeymapRule *rule)
{
    auto i = m_list.begin() + pos;
    m_list.insert(i, rule);
}

void
KeymapRulelist::append(OrderedKeymapRule *rule)
{
    m_list.push_back(rule);
}

void
KeymapRulelist::clear()
{
    for (auto i: m_list)
        delete i;

    m_list.clear();
}
