// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "graph.h"
#include "edgesinglechar.h"
#include "edgesinglenum.h"
#include "edgemultinum.h"
#include "edgesingletext.h"
#include "xterm.h"

// EDGE_SINGLE_NUMERIC '\xff'
// EDGE_MULTI_NUMERIC  '\xfe'
// EDGE_SINGLE_TEXT    '\xfd'
// EDGE_SINGLE_CHAR    '\xfc'

struct command {
    const char *sequence;
    void (XTermEmulator::*slot)();
    const char *slotName;
};
#define DEF(x, y) { x, &XTermEmulator::y, #y }

static const struct command s_commands[] = {
    DEF("\x1b F", cmdDisable8BitControls),
    DEF("\x1b G", cmdEnable8BitControls),
    DEF("\x1b#3", cmdDECDoubleHeightTop),
    DEF("\x1b#4", cmdDECDoubleHeightBottom),
    DEF("\x1b#5", cmdDECSingleWidth),
    DEF("\x1b#6", cmdDECDoubleWidth),
    DEF("\x1b#8", cmdDECScreenAlignmentTest),
    DEF("\x1b%\xfc", cmdIgnored),
    DEF("\x1b(\xfc", cmdDesignateCharset94),
    DEF("\x1b)\xfc", cmdDesignateCharset94),
    DEF("\x1b*\xfc", cmdDesignateCharset94),
    DEF("\x1b+\xfc", cmdDesignateCharset94),
    DEF("\x1b-\xfc", cmdDesignateCharset96),
    DEF("\x1b.\xfc", cmdDesignateCharset96),
    DEF("\x1b/\xfc", cmdDesignateCharset96),
    DEF("\x1b""7", cmdSaveCursor),
    DEF("\x1b""8", cmdRestoreCursor),
    DEF("\x1b=", cmdApplicationKeypad),
    DEF("\x1b>", cmdNormalKeypad),
    DEF("\x1b""c", cmdResetEmulator),
    DEF("\x1bn", cmdInvokeCharset),
    DEF("\x1bo", cmdInvokeCharset),
    DEF("\x1b|", cmdInvokeCharset),
    DEF("\x1b}", cmdInvokeCharset),
    DEF("\x1b~", cmdInvokeCharset),

    DEF("\x9b\xff@", cmdInsertCharacters),
    DEF("\x9b\xff""A", cmdCursorUp),
    DEF("\x9b\xff""B", cmdCursorDown),
    DEF("\x9b\xff""C", cmdCursorForward),
    DEF("\x9b\xff""D", cmdCursorBackward),
    DEF("\x9b\xff""E", cmdCursorNextLine),
    DEF("\x9b\xff""F", cmdCursorPreviousLine),
    DEF("\x9b\xffG", cmdCursorHorizontalAbsolute),
    DEF("\x9b\xfe;H", cmdCursorPosition),
    DEF("\x9b\xffI", cmdTabForward),
    DEF("\x9b\xffJ", cmdEraseInDisplay),
    DEF("\x9b?\xffJ", cmdSelectiveEraseInDisplay),
    DEF("\x9b\xffK", cmdEraseInLine),
    DEF("\x9b?\xffK", cmdSelectiveEraseInLine),
    DEF("\x9b\xffL", cmdInsertLines),
    DEF("\x9b\xffM", cmdDeleteLines),
    DEF("\x9b\xffP", cmdDeleteCharacters),
    DEF("\x9b\xffS", cmdScrollUp),
    DEF("\x9b\xffT", cmdScrollDown),
    DEF("\x9b>\xfe;T", cmdResetTitleModes),
    DEF("\x9b\xffX", cmdEraseCharacters),
    DEF("\x9b\xffZ", cmdTabBackward),
    DEF("\x9b\xff`", cmdCursorHorizontalAbsolute),
    DEF("\x9b\xff""a", cmdCursorForward),
    DEF("\x9b\xff""b", cmdRepeatCharacter),
    DEF("\x9b\xff""c", cmdSendDeviceAttributes),
    DEF("\x9b>\xff""c", cmdSendDeviceAttributes2),
    DEF("\x9b\xff""d", cmdCursorVerticalAbsolute),
    DEF("\x9b\xff""e", cmdCursorDown),
    DEF("\x9b\xfe;f", cmdCursorPosition),
    DEF("\x9b\xff""g", cmdTabClear),
    DEF("\x9b\xfe;h", cmdSetMode),
    DEF("\x9b?\xfe;h", cmdDECPrivateModeSet),
    DEF("\x9b\xfe;l", cmdResetMode),
    DEF("\x9b?\xfe;l", cmdDECPrivateModeReset),
    DEF("\x9b\xfe;m", cmdCharacterAttributes),
    DEF("\x9b\xffn", cmdDeviceStatusReport),
    DEF("\x9b!p", cmdResetEmulator),
    DEF("\x9b\xff$p", cmdModeRequest),
    DEF("\x9b?\xff$p", cmdDECPrivateModeRequest),
    DEF("\x9b\xfe;\"p", cmdIgnored),
    DEF("\x9b\xff\"q", cmdProtectionAttribute),
    DEF("\x9b\xff q", cmdSetCursorStyle),
    DEF("\x9b\xfe;r", cmdSetTopBottomMargins),
    DEF("\x9b?\xfe;r", cmdDECPrivateModeRestore),
    DEF("\x9b\xfe;s", cmdSetLeftRightMargins),
    DEF("\x9b?\xfe;s", cmdDECPrivateModeSave),
    DEF("\x9b\xfe;t", cmdWindowOps),
    DEF("\x9b>\xfe;t", cmdSetTitleModes),
    DEF("\x9bu", cmdRestoreCursor),

    DEF("\x90+p\xfd\x9c", cmdIgnored),
    DEF("\x90+q\xfd\x9c", cmdIgnored),
    DEF("\x90\xff;\xff|\xfd\x9c", cmdIgnored),
    DEF("\x90$q\xfd\x9c", dcsRequestStatusString),
    DEF("\x9d\xff\x07", oscMain),
    DEF("\x9d\xff\x9c", oscMain),
    DEF("\x9d\xff;\xfd\x07", oscMain),
    DEF("\x9d\xff;\xfd\x9c", oscMain),
    DEF("\x9e\xfd\x9c", cmdIgnored),
    DEF("\x9f\xfd\x9c", cmdIgnored),
    { NULL }
};

XTermNode *
XTermRootNode::addLiteralEdge(XTermNode *cur, Codepoint val)
{
    XTermNode::EdgeMap &edgeMap = cur->edgeMap;

    auto i = edgeMap.find(val);
    if (i != edgeMap.end()) {
        return i->second->next;
    }

    XTermEdge *edge = new XTermEdge();
    m_edges.push_back(edge);
    XTermNode *node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    edge->next = node;
    edgeMap.emplace(val, edge);
    return node;
}

XTermNode *
XTermRootNode::addSingleCharEdge(XTermNode *cur, int varnum)
{
    XTermEdge *edge = new XTermSingleCharEdge(varnum);
    m_edges.push_back(edge);
    XTermNode *node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    edge->next = node;
    cur->edgeList.insert(cur->edgeList.begin(), edge);
    return node;
}

XTermNode *
XTermRootNode::addSingleNumericEdge(XTermNode *cur, int varnum, Codepoint terminator)
{
    XTermEdge *edge;
    XTermNode *node;
    XTermNode::EdgeList &edgeList = cur->edgeList;
    XTermNode::EdgeList::const_iterator i = edgeList.begin();
    XTermNode::EdgeList::const_iterator j = edgeList.end();

    for (; i != j; ++i) {
        edge = *i;
        if (edge->type() == EDGE_SINGLE_NUMERIC) {
            static_cast<XTermSingleNumericEdge*>(edge)->addTerminator(terminator);
            return addLiteralEdge(edge->next, terminator);
        }
    }

    edge = new XTermSingleNumericEdge(varnum, terminator);
    m_edges.push_back(edge);
    node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    edge->next = node;
    cur->edgeList.insert(cur->edgeList.begin(), edge);
    return addLiteralEdge(node, terminator);
}

XTermNode *
XTermRootNode::addMultiNumericEdge(XTermNode *cur, int varnum, Codepoint separator, Codepoint terminator)
{
    XTermEdge *edge;
    XTermNode *node;
    XTermNode::EdgeList &edgeList = cur->edgeList;
    XTermNode::EdgeList::const_iterator i = edgeList.begin();
    XTermNode::EdgeList::const_iterator j = edgeList.end();

    for (; i != j; ++i) {
        edge = *i;
        if (edge->type() == EDGE_MULTI_NUMERIC) {
            static_cast<XTermMultiNumericEdge*>(edge)->addTerminator(terminator);
            return addLiteralEdge(edge->next, terminator);
        }
    }

    edge = new XTermMultiNumericEdge(varnum, separator, terminator);
    m_edges.push_back(edge);
    node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    edge->next = node;
    cur->edgeList.insert(cur->edgeList.begin(), edge);
    return addLiteralEdge(node, terminator);
}

XTermNode *
XTermRootNode::addSingleTextEdge(XTermNode *cur, int varnum, Codepoint terminator)
{
    XTermEdge *edge;
    XTermNode *node;
    XTermNode::EdgeList &edgeList = cur->edgeList;
    XTermNode::EdgeList::const_iterator i = edgeList.begin();
    XTermNode::EdgeList::const_iterator j = edgeList.end();

    for (; i != j; ++i) {
        edge = *i;
        if (edge->type() == EDGE_SINGLE_TEXT) {
            static_cast<XTermSingleTextEdge*>(edge)->addTerminator(terminator);
            return addLiteralEdge(edge->next, terminator);
        }
    }

    edge = new XTermSingleTextEdge(varnum, terminator);
    m_edges.push_back(edge);
    node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    edge->next = node;
    cur->edgeList.insert(cur->edgeList.begin(), edge);
    return addLiteralEdge(node, terminator);
}

void
XTermRootNode::addCommand(XTermNode *root, const struct command *command)
{
    XTermNode *cur = root;
    const char *ptr = command->sequence;
    int varnum = 0;
    unsigned char separator, terminus;

    while (*ptr) {
        switch (*ptr) {
        case EDGE_SINGLE_NUMERIC:
            terminus = *++ptr;
            cur = addSingleNumericEdge(cur, varnum++, terminus);
            break;
        case EDGE_MULTI_NUMERIC:
            separator = *++ptr;
            terminus = *++ptr;
            cur = addMultiNumericEdge(cur, varnum++, separator, terminus);
            break;
        case EDGE_SINGLE_TEXT:
            terminus = *++ptr;
            cur = addSingleTextEdge(cur, varnum++, terminus);
            break;
        case EDGE_SINGLE_CHAR:
            cur = addSingleCharEdge(cur, varnum++);
            break;
        default:
            cur = addLiteralEdge(cur, (unsigned char)*ptr);
            break;
        }

        ++ptr;
    }

    cur->isLeaf = true;
    cur->setSlot(command->slot);
    cur->setSlotName(command->slotName);
}

XTermRootNode::XTermRootNode(): XTermNode(&e_control)
{
    // Remove default edges
    edgeList.clear();

    // Add commands
    for (int i = 0; s_commands[i].sequence; ++i)
    {
        addCommand(this, s_commands + i);
    }

    // Control edge
    XTermNode *node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    node->isLeaf = true;
    node->setSlot(&XTermEmulator::process);
    node->setSlotName("process");
    e_control.next = node;

    // Print edge
    XTermEdge *edge = new XTermEdge();
    m_edges.push_back(edge);
    node = new XTermNode(&e_control);
    m_nodes.push_back(node);

    node->isLeaf = true;
    node->setSlot(&XTermEmulator::process);
    node->setSlotName("process");
    edge->next = node;
    edgeList.push_back(edge);
}

XTermRootNode::~XTermRootNode()
{
    for (auto i: m_nodes)
        delete i;
    for (auto i: m_edges)
        delete i;
}
