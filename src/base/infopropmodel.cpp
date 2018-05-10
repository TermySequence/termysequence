// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "infopropmodel.h"
#include "infoanim.h"
#include "term.h"
#include "server.h"
#include "screen.h"
#include "buffers.h"
#include "settings/servinfo.h"

#include <QHeaderView>

#define TR_TEXT1 TL("window-text", "No")
#define TR_TEXT2 TL("window-text", "Yes")
#define TR_TITLE1 TL("window-title", "Terminal Information") + A(": ")
#define TR_TITLE2 TL("window-title", "Server Information") + A(": ")

#define INFO_COLUMN_KEY         0
#define INFO_COLUMN_VALUE       1
#define INFO_N_COLUMNS          2

static const QString s_hex(L("0x%1"));

//
// Base Model
//

InfoPropModel::InfoPropModel(unsigned nIndexes, IdBase *idbase, QWidget *parent) :
    QAbstractItemModel(parent),
    m_values(nIndexes),
    m_nIndexes(nIndexes)
{
    m_values[0].first = idbase->idStr();
}

bool
InfoPropModel::updateData(int idx, const QString &value)
{
    auto &ref = m_values[idx];

    if (ref.first != value) {
        ref.first = value;

        QModelIndex start = indexForIndex(idx, INFO_COLUMN_VALUE);
        emit dataChanged(start, start);

        if (m_visible) {
            if (!ref.second) {
                ref.second = new InfoAnimation(this, idx);
                connect(ref.second, SIGNAL(animationSignal(intptr_t)),
                        SLOT(handleAnimation(intptr_t)));
            }
            ref.second->startColor(static_cast<QWidget*>(QObject::parent()));
        }
        return true;
    }
    return false;
}

inline void
InfoPropModel::updateList(int idx, QString value)
{
    updateData(idx, value.replace('\x1f', ' '));
}

inline void
InfoPropModel::updateFlag(int idx, uint64_t flags, uint64_t flag)
{
    updateData(idx, C((flags & flag) ? '1' : '0'));
}

inline void
InfoPropModel::updateChar(int idx, const char *chars, int charidx)
{
    updateData(idx, s_hex.arg((unsigned)chars[charidx], 2, 16, Ch0));
}

void
InfoPropModel::handleAnimation(intptr_t idx)
{
    QModelIndex start = indexForIndex(idx, INFO_COLUMN_VALUE);
    emit dataChanged(start, start, QVector<int>(1, Qt::BackgroundRole));
}

/*
 * Model functions
 */
int
InfoPropModel::columnCount(const QModelIndex &parent) const
{
    return INFO_N_COLUMNS;
}

int
InfoPropModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_nRows;
    if (!parent.parent().isValid())
        return m_children[parent.row()];

    return 0;
}

QVariant
InfoPropModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case INFO_COLUMN_KEY:
            return tr("Property", "heading");
        case INFO_COLUMN_VALUE:
            return tr("Value", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
InfoPropModel::data(const QModelIndex &index, int role) const
{
    int idx;
    QModelIndex parent = index.parent();

    if (parent.isValid())
        idx = index.row() + m_counts[parent.row()] + 1;
    else
        idx = m_counts[index.row()];

    if (index.column() == INFO_COLUMN_KEY) {
        switch (role) {
        case Qt::DisplayRole:
            return getRowName(idx);
        }
    }
    else {
        auto &ref = m_values[idx];

        switch (role) {
        case Qt::DisplayRole:
            return ref.first;
        case Qt::BackgroundRole:
            return ref.second ? ref.second->colorVariant() : QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags
InfoPropModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

QModelIndex
InfoPropModel::parent(const QModelIndex &index) const
{
    intptr_t parentRow = index.internalId();

    if (parentRow != -1l)
        return createIndex(parentRow, 0, -1l);
    else
        return QModelIndex();
}

QModelIndex
InfoPropModel::index(int row, int column, const QModelIndex &parent) const
{
    intptr_t id;
    int bound;

    if (parent.isValid()) {
        id = parent.row();
        bound = m_children[parent.row()];
    } else {
        id = -1l;
        bound = m_nRows;
    }

    return (row < bound) ? createIndex(row, column, id) : QModelIndex();
}

QModelIndex
InfoPropModel::indexForIndex(int idx, int col) const
{
    intptr_t parentRow = m_indexes[idx * 2];
    int childRow = m_indexes[idx * 2 + 1];

    if (childRow == -1) {
        return createIndex(parentRow, col, -1l);
    } else {
        return createIndex(childRow, col, parentRow);
    }
}

//
// Term Model
//
enum TermRowNumber {
    TERM_ROW_ID,
    TERM_ROW_TITLE,
    TERM_ROW_TITLE2,
    TERM_ROW_WIDTH,
    TERM_ROW_HEIGHT,
    TERM_ROW_ENCODING,
    TERM_ROW_SI_VERSION,
    TERM_ROW_SI_SHELL,
    TERM_ROW_SB_ENABLED,
    TERM_ROW_SB_CAPACITY,
    TERM_ROW_SB_SIZE,
    TERM_ROW_SB_POSITION,
    TERM_ROW_SB_FETCHPOS,
    TERM_ROW_SB_FETCHNEXT,
    TERM_ROW_THROTTLED,
    TERM_ROW_FLAGS,
    TERM_ROW_IOS_INPUT,
    TERM_ROW_IOS_OUTPUT,
    TERM_ROW_IOS_LOCAL,
    TERM_ROW_IOS_CHARS,
    TERM_ROW_PROC_PID,
    TERM_ROW_PROC_CWD,
    TERM_ROW_PROC_COMM,
    TERM_ROW_PROC_STATUS,
    TERM_ROW_PROC_OUTCOME,
    TERM_N_ROWS,
};

enum TermIndexNumber {
    TERM_IDX_ID,
    TERM_IDX_TITLE,
    TERM_IDX_TITLE2,
    TERM_IDX_WIDTH,
    TERM_IDX_HEIGHT,
    TERM_IDX_ENCODING,
    TERM_IDX_SI_VERSION,
    TERM_IDX_SI_SHELL,
    TERM_IDX_SB_ENABLED,
    TERM_IDX_SB_CAPACITY,
    TERM_IDX_SB_SIZE,
    TERM_IDX_SB_POSITION,
    TERM_IDX_SB_FETCHPOS,
    TERM_IDX_SB_FETCHNEXT,
    TERM_IDX_THROTTLED,
    TERM_IDX_FLAGS,
    TERM_IDX_FL_ANSI,
    TERM_IDX_FL_NEWLINE,
    TERM_IDX_FL_APPCUKEYS,
    TERM_IDX_FL_APPSCREEN,
    TERM_IDX_FL_APPKEYPAD,
    TERM_IDX_FL_REVERSEVIDEO,
    TERM_IDX_FL_BLINKSEEN,
    TERM_IDX_FL_HARDSCROLLLOCK,
    TERM_IDX_FL_SOFTSCROLLLOCK,
    TERM_IDX_FL_KEYBOARDLOCK,
    TERM_IDX_FL_SENDRECEIVE,
    TERM_IDX_FL_INSERTMODE,
    TERM_IDX_FL_LRMMODE,
    TERM_IDX_FL_ORIGINMODE,
    TERM_IDX_FL_SMOOTHSCROLLING,
    TERM_IDX_FL_AUTOWRAP,
    TERM_IDX_FL_REVERSEAUTOWRAP,
    TERM_IDX_FL_AUTOREPEAT,
    TERM_IDX_FL_ALLOWCOLUMNCHANGE,
    TERM_IDX_FL_CONTROLS8BIT,
    TERM_IDX_FL_BRACKETEDPASTE,
    TERM_IDX_FL_CURSORVISIBLE,
    TERM_IDX_FL_X10MOUSE,
    TERM_IDX_FL_NORMALMOUSE,
    TERM_IDX_FL_HIGHLIGHTMOUSE,
    TERM_IDX_FL_BUTTONEVENTMOUSE,
    TERM_IDX_FL_ANYEVENTMOUSE,
    TERM_IDX_FL_FOCUSEVENT,
    TERM_IDX_FL_UTF8EXTMOUSE,
    TERM_IDX_FL_SGREXTMOUSE,
    TERM_IDX_FL_URXVTEXTMOUSE,
    TERM_IDX_FL_ALTSCROLLMOUSE,
    TERM_IDX_IOS_INPUT,
    TERM_IDX_IOS_IGNBRK,
    TERM_IDX_IOS_BRKINT,
    TERM_IDX_IOS_IGNPAR,
    TERM_IDX_IOS_PARMRK,
    TERM_IDX_IOS_INPCK,
    TERM_IDX_IOS_ISTRIP,
    TERM_IDX_IOS_INLCR,
    TERM_IDX_IOS_IGNCR,
    TERM_IDX_IOS_ICRNL,
    TERM_IDX_IOS_IUCLC,
    TERM_IDX_IOS_IXON,
    TERM_IDX_IOS_IXANY,
    TERM_IDX_IOS_IXOFF,
    TERM_IDX_IOS_OUTPUT,
    TERM_IDX_IOS_OPOST,
    TERM_IDX_IOS_OLCUC,
    TERM_IDX_IOS_ONLCR,
    TERM_IDX_IOS_OCRNL,
    TERM_IDX_IOS_ONOCR,
    TERM_IDX_IOS_ONLRET,
    TERM_IDX_IOS_OXTABS,
    TERM_IDX_IOS_LOCAL,
    TERM_IDX_IOS_ISIG,
    TERM_IDX_IOS_ICANON,
    TERM_IDX_IOS_ECHO,
    TERM_IDX_IOS_ECHOE,
    TERM_IDX_IOS_ECHOK,
    TERM_IDX_IOS_ECHONL,
    TERM_IDX_IOS_NOFLSH,
    TERM_IDX_IOS_TOSTOP,
    TERM_IDX_IOS_ECHOCTL,
    TERM_IDX_IOS_ECHOPRT,
    TERM_IDX_IOS_ECHOKE,
    TERM_IDX_IOS_FLUSHO,
    TERM_IDX_IOS_PENDIN,
    TERM_IDX_IOS_IEXTEN,
    TERM_IDX_IOS_CHARS,
    TERM_IDX_IOS_VEOF,
    TERM_IDX_IOS_VEOL,
    TERM_IDX_IOS_VEOL2,
    TERM_IDX_IOS_VERASE,
    TERM_IDX_IOS_VWERASE,
    TERM_IDX_IOS_VKILL,
    TERM_IDX_IOS_VREPRINT,
    TERM_IDX_IOS_VINTR,
    TERM_IDX_IOS_VQUIT,
    TERM_IDX_IOS_VSUSP,
    TERM_IDX_IOS_VDSUSP,
    TERM_IDX_IOS_VSTART,
    TERM_IDX_IOS_VSTOP,
    TERM_IDX_IOS_VLNEXT,
    TERM_IDX_IOS_VDISCARD,
    TERM_IDX_IOS_VMIN,
    TERM_IDX_IOS_VTIME,
    TERM_IDX_IOS_VSTATUS,
    TERM_IDX_PROC_PID,
    TERM_IDX_PROC_CWD,
    TERM_IDX_PROC_COMM,
    TERM_IDX_PROC_STATUS,
    TERM_IDX_PROC_OUTCOME,
    TERM_N_INDEXES,
};

static const int8_t s_termChildren[TERM_N_ROWS] = {
    0, /* id */
    0, /* title */
    0, /* title2 */
    0, /* width */
    0, /* height */
    0, /* encoding */
    0, /* si version */
    0, /* si shell */
    0, /* sb enabled */
    0, /* sb capacity */
    0, /* sb size */
    0, /* sb position */
    0, /* sb fetchpos */
    0, /* sb fetchnext */
    0, /* throttled */
    32, /* flags */
    13, /* ios input */
    7, /* ios output */
    14, /* ios local */
    18, /* ios chars */
    0, /* proc pid */
    0, /* proc cwd */
    0, /* proc comm */
    0, /* proc status */
    0, /* proc outcome */
};

static const int8_t s_termCounts[TERM_N_ROWS] = {
    TERM_IDX_ID,
    TERM_IDX_TITLE,
    TERM_IDX_TITLE2,
    TERM_IDX_WIDTH,
    TERM_IDX_HEIGHT,
    TERM_IDX_ENCODING,
    TERM_IDX_SI_VERSION,
    TERM_IDX_SI_SHELL,
    TERM_IDX_SB_ENABLED,
    TERM_IDX_SB_CAPACITY,
    TERM_IDX_SB_SIZE,
    TERM_IDX_SB_POSITION,
    TERM_IDX_SB_FETCHPOS,
    TERM_IDX_SB_FETCHNEXT,
    TERM_IDX_THROTTLED,
    TERM_IDX_FLAGS,
    TERM_IDX_IOS_INPUT,
    TERM_IDX_IOS_OUTPUT,
    TERM_IDX_IOS_LOCAL,
    TERM_IDX_IOS_CHARS,
    TERM_IDX_PROC_PID,
    TERM_IDX_PROC_CWD,
    TERM_IDX_PROC_COMM,
    TERM_IDX_PROC_STATUS,
    TERM_IDX_PROC_OUTCOME,
};

// Format: <parentRow> <childRow>
static const int8_t s_termIndexes[TERM_N_INDEXES * 2] = {
    TERM_ROW_ID, -1,
    TERM_ROW_TITLE, -1,
    TERM_ROW_TITLE2, -1,
    TERM_ROW_WIDTH, -1,
    TERM_ROW_HEIGHT, -1,
    TERM_ROW_ENCODING, -1,
    TERM_ROW_SI_VERSION, -1,
    TERM_ROW_SI_SHELL, -1,
    TERM_ROW_SB_ENABLED, -1,
    TERM_ROW_SB_CAPACITY, -1,
    TERM_ROW_SB_SIZE, -1,
    TERM_ROW_SB_POSITION, -1,
    TERM_ROW_SB_FETCHPOS, -1,
    TERM_ROW_SB_FETCHNEXT, -1,
    TERM_ROW_THROTTLED, -1,

    TERM_ROW_FLAGS, -1,
    TERM_ROW_FLAGS, 0, /* Ansi */
    TERM_ROW_FLAGS, 1, /* NewLine */
    TERM_ROW_FLAGS, 2, /* AppCuKeys */
    TERM_ROW_FLAGS, 3, /* AppScreen */
    TERM_ROW_FLAGS, 4, /* AppKeyPad */
    TERM_ROW_FLAGS, 5, /* ReverseVideo */
    TERM_ROW_FLAGS, 6, /* BlinkSeen */
    TERM_ROW_FLAGS, 7, /* HardScrollLock */
    TERM_ROW_FLAGS, 8, /* SoftScrollLock */
    TERM_ROW_FLAGS, 9, /* KeyboardLock */
    TERM_ROW_FLAGS, 10, /* SendReceive */
    TERM_ROW_FLAGS, 11, /* InsertMode */
    TERM_ROW_FLAGS, 12, /* LeftRightMarginMode */
    TERM_ROW_FLAGS, 13, /* OriginMode */
    TERM_ROW_FLAGS, 14, /* SmoothScrolling */
    TERM_ROW_FLAGS, 15, /* Autowrap */
    TERM_ROW_FLAGS, 16, /* ReverseAutowrap */
    TERM_ROW_FLAGS, 17, /* Autorepeat */
    TERM_ROW_FLAGS, 18, /* AllowColumnChange */
    TERM_ROW_FLAGS, 19, /* Controls8Bit */
    TERM_ROW_FLAGS, 20, /* BracketedPasteMode */
    TERM_ROW_FLAGS, 21, /* CursorVisible */
    TERM_ROW_FLAGS, 22, /* X10MouseMode */
    TERM_ROW_FLAGS, 23, /* NormalMouseMode */
    TERM_ROW_FLAGS, 24, /* HighlightMouseMode */
    TERM_ROW_FLAGS, 25, /* ButtonEventMouseMode */
    TERM_ROW_FLAGS, 26, /* AnyEventMouseMode */
    TERM_ROW_FLAGS, 27, /* FocusEventMode */
    TERM_ROW_FLAGS, 28, /* Utf8ExtMouseMode */
    TERM_ROW_FLAGS, 29, /* SgrExtMouseMode */
    TERM_ROW_FLAGS, 30, /* UrxvtExtMouseMode */
    TERM_ROW_FLAGS, 31, /* AltScrollMouseMode */

    TERM_ROW_IOS_INPUT, -1,
    TERM_ROW_IOS_INPUT, 0, /* TermiosIGNBRK */
    TERM_ROW_IOS_INPUT, 1, /* TermiosBRKINT */
    TERM_ROW_IOS_INPUT, 2, /* TermiosIGNPAR */
    TERM_ROW_IOS_INPUT, 3, /* TermiosPARMRK */
    TERM_ROW_IOS_INPUT, 4, /* TermiosINPCK */
    TERM_ROW_IOS_INPUT, 5, /* TermiosISTRIP */
    TERM_ROW_IOS_INPUT, 6, /* TermiosINLCR */
    TERM_ROW_IOS_INPUT, 7, /* TermiosIGNCR */
    TERM_ROW_IOS_INPUT, 8, /* TermiosICRNL */
    TERM_ROW_IOS_INPUT, 9, /* TermiosIUCLC */
    TERM_ROW_IOS_INPUT, 10, /* TermiosIXON */
    TERM_ROW_IOS_INPUT, 11, /* TermiosIXANY */
    TERM_ROW_IOS_INPUT, 12, /* TermiosIXOFF */

    TERM_ROW_IOS_OUTPUT, -1,
    TERM_ROW_IOS_OUTPUT, 0, /* TermiosOPOST */
    TERM_ROW_IOS_OUTPUT, 1, /* TermiosOLCUC */
    TERM_ROW_IOS_OUTPUT, 2, /* TermiosONLCR */
    TERM_ROW_IOS_OUTPUT, 3, /* TermiosOCRNL */
    TERM_ROW_IOS_OUTPUT, 4, /* TermiosONOCR */
    TERM_ROW_IOS_OUTPUT, 5, /* TermiosONLRET */
    TERM_ROW_IOS_OUTPUT, 6, /* TermiosOXTABS */

    TERM_ROW_IOS_LOCAL, -1,
    TERM_ROW_IOS_LOCAL, 0, /* TermiosISIG */
    TERM_ROW_IOS_LOCAL, 1, /* TermiosICANON */
    TERM_ROW_IOS_LOCAL, 2, /* TermiosECHO */
    TERM_ROW_IOS_LOCAL, 3, /* TermiosECHOE */
    TERM_ROW_IOS_LOCAL, 4, /* TermiosECHOK */
    TERM_ROW_IOS_LOCAL, 5, /* TermiosECHONL */
    TERM_ROW_IOS_LOCAL, 6, /* TermiosNOFLSH */
    TERM_ROW_IOS_LOCAL, 7, /* TermiosTOSTOP */
    TERM_ROW_IOS_LOCAL, 8, /* TermiosECHOCTL */
    TERM_ROW_IOS_LOCAL, 9, /* TermiosECHOPRT */
    TERM_ROW_IOS_LOCAL, 10, /* TermiosECHOKE */
    TERM_ROW_IOS_LOCAL, 11, /* TermiosFLUSHO */
    TERM_ROW_IOS_LOCAL, 12, /* TermiosPENDIN */
    TERM_ROW_IOS_LOCAL, 13, /* TermiosIEXTEN */

    TERM_ROW_IOS_CHARS, -1,
    TERM_ROW_IOS_CHARS, 0, /* TermiosVEOF */
    TERM_ROW_IOS_CHARS, 1, /* TermiosVEOL */
    TERM_ROW_IOS_CHARS, 2, /* TermiosVEOL2 */
    TERM_ROW_IOS_CHARS, 3, /* TermiosVERASE */
    TERM_ROW_IOS_CHARS, 4, /* TermiosVWERASE */
    TERM_ROW_IOS_CHARS, 5, /* TermiosVKILL */
    TERM_ROW_IOS_CHARS, 6, /* TermiosVREPRINT */
    TERM_ROW_IOS_CHARS, 7, /* TermiosVINTR */
    TERM_ROW_IOS_CHARS, 8, /* TermiosVQUIT */
    TERM_ROW_IOS_CHARS, 9, /* TermiosVSUSP */
    TERM_ROW_IOS_CHARS, 10, /* TermiosVDSUSP */
    TERM_ROW_IOS_CHARS, 11, /* TermiosVSTART */
    TERM_ROW_IOS_CHARS, 12, /* TermiosVSTOP */
    TERM_ROW_IOS_CHARS, 13, /* TermiosVLNEXT */
    TERM_ROW_IOS_CHARS, 14, /* TermiosVDISCARD */
    TERM_ROW_IOS_CHARS, 15, /* TermiosVMIN */
    TERM_ROW_IOS_CHARS, 16, /* TermiosVTIME */
    TERM_ROW_IOS_CHARS, 17, /* TermiosVSTATUS */

    TERM_ROW_PROC_PID, -1,
    TERM_ROW_PROC_CWD, -1,
    TERM_ROW_PROC_COMM, -1,
    TERM_ROW_PROC_STATUS, -1,
    TERM_ROW_PROC_OUTCOME, -1,
};

TermPropModel::TermPropModel(TermInstance *term, QWidget *parent) :
    InfoPropModel(TERM_N_INDEXES, term, parent),
    m_term(term)
{
    m_nRows = TERM_N_ROWS;
    m_counts = s_termCounts;
    m_children = s_termChildren;
    m_indexes = s_termIndexes;

    connect(term, SIGNAL(sizeChanged(QSize)), SLOT(handleSizeChanged(QSize)));
    connect(term->buffers(), SIGNAL(bufferReset()), SLOT(handleBufferReset()));
    connect(term->buffers(), SIGNAL(bufferChanged()), SLOT(handleBufferChanged()));
    connect(term->buffers(), SIGNAL(fetchPosChanged()), SLOT(handleFetchChanged()));
    connect(term, SIGNAL(attributeChanged(const QString&,const QString&)), SLOT(handleAttribute(const QString&)));
    connect(term, SIGNAL(attributeRemoved(const QString&)), SLOT(handleAttribute(const QString&)));
    connect(term, SIGNAL(flagsChanged(Tsq::TermFlags)), SLOT(handleFlagsChanged(Tsq::TermFlags)));
    connect(term, SIGNAL(processChanged(const QString&)), SLOT(handleProcessChanged()));
    connect(term, SIGNAL(throttleChanged(bool)), SLOT(handleThrottleChanged(bool)));

    handleSizeChanged(term->screen()->size());
    handleBufferReset();
    handleBufferChanged();
    handleFetchChanged();

    handleAttribute(g_attr_SESSION_TITLE);
    handleAttribute(g_attr_SESSION_TITLE2);
    handleAttribute(g_attr_ENCODING);
    handleAttribute(g_attr_SESSION_SIVERSION);
    handleAttribute(g_attr_SESSION_SISHELL);
    handleAttribute(g_attr_PROC_STATUS);
    handleFlagsChanged(term->flags());
    handleProcessChanged();
    handleThrottleChanged(term->throttled());
}

void
TermPropModel::handleSizeChanged(QSize size)
{
    QString sbWidth = QString::number(size.width());
    QString sbHeight = QString::number(size.height());
    updateData(TERM_IDX_WIDTH, sbWidth);
    updateData(TERM_IDX_HEIGHT, sbHeight);
}

void
TermPropModel::handleBufferReset()
{
    QString sbDisabled = m_term->buffers()->noScrollback() ? TR_TEXT1 : TR_TEXT2;
    QString sbCapacity = QString::number(1 << m_term->buffers()->caporder());
    updateData(TERM_IDX_SB_ENABLED, sbDisabled);
    updateData(TERM_IDX_SB_CAPACITY, sbCapacity);
}

void
TermPropModel::handleBufferChanged()
{
    QString sbSize = QString::number(m_term->buffers()->size());
    QString sbPosition = QString::number(m_term->buffers()->origin());
    updateData(TERM_IDX_SB_SIZE, sbSize);
    updateData(TERM_IDX_SB_POSITION, sbPosition);
}

void
TermPropModel::handleFetchChanged()
{
    QString fetchPos = QString::number(m_term->buffers()->fetchPos());
    QString fetchNext;

    index_t fn = m_term->buffers()->fetchNext();
    if (fn != INVALID_INDEX)
        fetchNext = QString::number(fn);

    updateData(TERM_IDX_SB_FETCHPOS, fetchPos);
    updateData(TERM_IDX_SB_FETCHNEXT, fetchNext);
}

void
TermPropModel::handleAttribute(const QString &key)
{
    if (key == g_attr_SESSION_TITLE) {
        QString title = m_term->attributes().value(key);
        updateData(TERM_IDX_TITLE, title);
        static_cast<QWidget*>(QObject::parent())->setWindowTitle(TR_TITLE1 + title);
    }
    else if (key == g_attr_SESSION_TITLE2)
        updateData(TERM_IDX_TITLE2, m_term->attributes().value(key));
    else if (key == g_attr_PROC_STATUS)
        switch (m_term->attributes().value(key).toInt()) {
        case Tsq::TermIdle:
            updateData(TERM_IDX_PROC_STATUS, L("Idle"));
            break;
        case Tsq::TermActive:
            updateData(TERM_IDX_PROC_STATUS, L("Running"));
            break;
        case Tsq::TermBusy:
            updateData(TERM_IDX_PROC_STATUS, L("Busy"));
            break;
        default:
            updateData(TERM_IDX_PROC_STATUS, L("Closed"));
            break;
        }
    else if (key == g_attr_ENCODING)
        updateList(TERM_IDX_ENCODING, m_term->attributes().value(key));
    else if (key == g_attr_SESSION_SIVERSION)
        updateData(TERM_IDX_SI_VERSION, m_term->attributes().value(key));
    else if (key == g_attr_SESSION_SISHELL)
        updateData(TERM_IDX_SI_SHELL, m_term->attributes().value(key));
}

void
TermPropModel::handleFlagsChanged(Tsq::TermFlags flags)
{
    QString flagStr = s_hex.arg(flags & ~Tsq::LocalTermMask, 0, 16);

    if (updateData(TERM_IDX_FLAGS, flagStr)) {
        updateFlag(TERM_IDX_FL_ANSI, flags, Tsq::Ansi);
        updateFlag(TERM_IDX_FL_NEWLINE, flags, Tsq::NewLine);
        updateFlag(TERM_IDX_FL_APPCUKEYS, flags, Tsq::AppCuKeys);
        updateFlag(TERM_IDX_FL_APPSCREEN, flags, Tsq::AppScreen);
        updateFlag(TERM_IDX_FL_APPKEYPAD, flags, Tsq::AppKeyPad);
        updateFlag(TERM_IDX_FL_REVERSEVIDEO, flags, Tsq::ReverseVideo);
        updateFlag(TERM_IDX_FL_BLINKSEEN, flags, Tsq::BlinkSeen);
        updateFlag(TERM_IDX_FL_HARDSCROLLLOCK, flags, Tsq::HardScrollLock);
        updateFlag(TERM_IDX_FL_SOFTSCROLLLOCK, flags, Tsq::SoftScrollLock);
        updateFlag(TERM_IDX_FL_KEYBOARDLOCK, flags, Tsq::KeyboardLock);
        updateFlag(TERM_IDX_FL_SENDRECEIVE, flags, Tsq::SendReceive);
        updateFlag(TERM_IDX_FL_INSERTMODE, flags, Tsq::InsertMode);
        updateFlag(TERM_IDX_FL_LRMMODE, flags, Tsq::LeftRightMarginMode);
        updateFlag(TERM_IDX_FL_ORIGINMODE, flags, Tsq::OriginMode);
        updateFlag(TERM_IDX_FL_SMOOTHSCROLLING, flags, Tsq::SmoothScrolling);
        updateFlag(TERM_IDX_FL_AUTOWRAP, flags, Tsq::Autowrap);
        updateFlag(TERM_IDX_FL_REVERSEAUTOWRAP, flags, Tsq::ReverseAutowrap);
        updateFlag(TERM_IDX_FL_AUTOREPEAT, flags, Tsq::Autorepeat);
        updateFlag(TERM_IDX_FL_ALLOWCOLUMNCHANGE, flags, Tsq::AllowColumnChange);
        updateFlag(TERM_IDX_FL_CONTROLS8BIT, flags, Tsq::Controls8Bit);
        updateFlag(TERM_IDX_FL_BRACKETEDPASTE, flags, Tsq::BracketedPasteMode);
        updateFlag(TERM_IDX_FL_CURSORVISIBLE, flags, Tsq::CursorVisible);
        updateFlag(TERM_IDX_FL_X10MOUSE, flags, Tsq::X10MouseMode);
        updateFlag(TERM_IDX_FL_NORMALMOUSE, flags, Tsq::NormalMouseMode);
        updateFlag(TERM_IDX_FL_HIGHLIGHTMOUSE, flags, Tsq::HighlightMouseMode);
        updateFlag(TERM_IDX_FL_BUTTONEVENTMOUSE, flags, Tsq::ButtonEventMouseMode);
        updateFlag(TERM_IDX_FL_ANYEVENTMOUSE, flags, Tsq::AnyEventMouseMode);
        updateFlag(TERM_IDX_FL_FOCUSEVENT, flags, Tsq::FocusEventMode);
        updateFlag(TERM_IDX_FL_UTF8EXTMOUSE, flags, Tsq::Utf8ExtMouseMode);
        updateFlag(TERM_IDX_FL_SGREXTMOUSE, flags, Tsq::SgrExtMouseMode);
        updateFlag(TERM_IDX_FL_URXVTEXTMOUSE, flags, Tsq::UrxvtExtMouseMode);
        updateFlag(TERM_IDX_FL_ALTSCROLLMOUSE, flags, Tsq::AltScrollMouseMode);
    }
}

void
TermPropModel::handleProcessChanged()
{
    const auto *process = m_term->process();
    updateData(TERM_IDX_PROC_PID, QString::number(process->pid));
    updateData(TERM_IDX_PROC_CWD, process->workingDir);
    updateData(TERM_IDX_PROC_COMM, process->commandName);
    updateData(TERM_IDX_PROC_OUTCOME, process->outcomeStr);

    QString inputStr, outputStr, localStr;
    inputStr = s_hex.arg(process->termiosInputFlags, 0, 16);
    outputStr = s_hex.arg(process->termiosOutputFlags, 0, 16);
    localStr = s_hex.arg(process->termiosLocalFlags, 0, 16);

    if (updateData(TERM_IDX_IOS_INPUT, inputStr)) {
        updateFlag(TERM_IDX_IOS_IGNBRK, process->termiosInputFlags, Tsq::TermiosIGNBRK);
        updateFlag(TERM_IDX_IOS_BRKINT, process->termiosInputFlags, Tsq::TermiosBRKINT);
        updateFlag(TERM_IDX_IOS_IGNPAR, process->termiosInputFlags, Tsq::TermiosIGNPAR);
        updateFlag(TERM_IDX_IOS_PARMRK, process->termiosInputFlags, Tsq::TermiosPARMRK);
        updateFlag(TERM_IDX_IOS_INPCK, process->termiosInputFlags, Tsq::TermiosINPCK);
        updateFlag(TERM_IDX_IOS_ISTRIP, process->termiosInputFlags, Tsq::TermiosISTRIP);
        updateFlag(TERM_IDX_IOS_INLCR, process->termiosInputFlags, Tsq::TermiosINLCR);
        updateFlag(TERM_IDX_IOS_IGNCR, process->termiosInputFlags, Tsq::TermiosIGNCR);
        updateFlag(TERM_IDX_IOS_ICRNL, process->termiosInputFlags, Tsq::TermiosICRNL);
        updateFlag(TERM_IDX_IOS_IUCLC, process->termiosInputFlags, Tsq::TermiosIUCLC);
        updateFlag(TERM_IDX_IOS_IXON, process->termiosInputFlags, Tsq::TermiosIXON);
        updateFlag(TERM_IDX_IOS_IXANY, process->termiosInputFlags, Tsq::TermiosIXANY);
        updateFlag(TERM_IDX_IOS_IXOFF, process->termiosInputFlags, Tsq::TermiosIXOFF);
    }
    if (updateData(TERM_IDX_IOS_OUTPUT, outputStr)) {
        updateFlag(TERM_IDX_IOS_OPOST, process->termiosOutputFlags, Tsq::TermiosOPOST);
        updateFlag(TERM_IDX_IOS_OLCUC, process->termiosOutputFlags, Tsq::TermiosOLCUC);
        updateFlag(TERM_IDX_IOS_ONLCR, process->termiosOutputFlags, Tsq::TermiosONLCR);
        updateFlag(TERM_IDX_IOS_OCRNL, process->termiosOutputFlags, Tsq::TermiosOCRNL);
        updateFlag(TERM_IDX_IOS_ONOCR, process->termiosOutputFlags, Tsq::TermiosONOCR);
        updateFlag(TERM_IDX_IOS_ONLRET, process->termiosOutputFlags, Tsq::TermiosONLRET);
        updateFlag(TERM_IDX_IOS_OXTABS, process->termiosOutputFlags, Tsq::TermiosOXTABS);
    }
    if (updateData(TERM_IDX_IOS_LOCAL, localStr)) {
        updateFlag(TERM_IDX_IOS_ISIG, process->termiosLocalFlags, Tsq::TermiosISIG);
        updateFlag(TERM_IDX_IOS_ICANON, process->termiosLocalFlags, Tsq::TermiosICANON);
        updateFlag(TERM_IDX_IOS_ECHO, process->termiosLocalFlags, Tsq::TermiosECHO);
        updateFlag(TERM_IDX_IOS_ECHOE, process->termiosLocalFlags, Tsq::TermiosECHOE);
        updateFlag(TERM_IDX_IOS_ECHOK, process->termiosLocalFlags, Tsq::TermiosECHOK);
        updateFlag(TERM_IDX_IOS_ECHONL, process->termiosLocalFlags, Tsq::TermiosECHONL);
        updateFlag(TERM_IDX_IOS_NOFLSH, process->termiosLocalFlags, Tsq::TermiosNOFLSH);
        updateFlag(TERM_IDX_IOS_TOSTOP, process->termiosLocalFlags, Tsq::TermiosTOSTOP);
        updateFlag(TERM_IDX_IOS_ECHOCTL, process->termiosLocalFlags, Tsq::TermiosECHOCTL);
        updateFlag(TERM_IDX_IOS_ECHOPRT, process->termiosLocalFlags, Tsq::TermiosECHOPRT);
        updateFlag(TERM_IDX_IOS_ECHOKE, process->termiosLocalFlags, Tsq::TermiosECHOKE);
        updateFlag(TERM_IDX_IOS_FLUSHO, process->termiosLocalFlags, Tsq::TermiosFLUSHO);
        updateFlag(TERM_IDX_IOS_PENDIN, process->termiosLocalFlags, Tsq::TermiosPENDIN);
        updateFlag(TERM_IDX_IOS_IEXTEN, process->termiosLocalFlags, Tsq::TermiosIEXTEN);
    }

    updateChar(TERM_IDX_IOS_VEOF, process->termiosChars, Tsq::TermiosVEOF);
    updateChar(TERM_IDX_IOS_VEOL, process->termiosChars, Tsq::TermiosVEOL);
    updateChar(TERM_IDX_IOS_VEOL2, process->termiosChars, Tsq::TermiosVEOL2);
    updateChar(TERM_IDX_IOS_VERASE, process->termiosChars, Tsq::TermiosVERASE);
    updateChar(TERM_IDX_IOS_VWERASE, process->termiosChars, Tsq::TermiosVWERASE);
    updateChar(TERM_IDX_IOS_VKILL, process->termiosChars, Tsq::TermiosVKILL);
    updateChar(TERM_IDX_IOS_VREPRINT, process->termiosChars, Tsq::TermiosVREPRINT);
    updateChar(TERM_IDX_IOS_VINTR, process->termiosChars, Tsq::TermiosVINTR);
    updateChar(TERM_IDX_IOS_VQUIT, process->termiosChars, Tsq::TermiosVQUIT);
    updateChar(TERM_IDX_IOS_VSUSP, process->termiosChars, Tsq::TermiosVSUSP);
    updateChar(TERM_IDX_IOS_VDSUSP, process->termiosChars, Tsq::TermiosVDSUSP);
    updateChar(TERM_IDX_IOS_VSTART, process->termiosChars, Tsq::TermiosVSTART);
    updateChar(TERM_IDX_IOS_VSTOP, process->termiosChars, Tsq::TermiosVSTOP);
    updateChar(TERM_IDX_IOS_VLNEXT, process->termiosChars, Tsq::TermiosVLNEXT);
    updateChar(TERM_IDX_IOS_VDISCARD, process->termiosChars, Tsq::TermiosVDISCARD);
    updateChar(TERM_IDX_IOS_VMIN, process->termiosChars, Tsq::TermiosVMIN);
    updateChar(TERM_IDX_IOS_VTIME, process->termiosChars, Tsq::TermiosVTIME);
    updateChar(TERM_IDX_IOS_VSTATUS, process->termiosChars, Tsq::TermiosVSTATUS);
}

void
TermPropModel::handleThrottleChanged(bool throttled)
{
    QString throttleStr = throttled ? TR_TEXT2 : TR_TEXT1;
    updateData(TERM_IDX_THROTTLED, throttleStr);
}

QString
TermPropModel::getRowName(int idx) const
{
    switch (idx) {
    case TERM_IDX_ID: return tr("Identifier", "property-name");
    case TERM_IDX_TITLE: return tr("Window Title", "property-name");
    case TERM_IDX_TITLE2: return tr("Window Icon Name", "property-name");
    case TERM_IDX_WIDTH: return tr("Width", "property-name");
    case TERM_IDX_HEIGHT: return tr("Height", "property-name");
    case TERM_IDX_ENCODING: return tr("Encoding", "property-name");
    case TERM_IDX_SI_VERSION: return tr("Shell Integration Version", "property-name");
    case TERM_IDX_SI_SHELL: return tr("Shell Integration Name", "property-name");
    case TERM_IDX_SB_ENABLED: return tr("Scrollback Enabled", "property-name");
    case TERM_IDX_SB_CAPACITY: return tr("Scrollback Capacity", "property-name");
    case TERM_IDX_SB_SIZE: return tr("Scrollback Size", "property-name");
    case TERM_IDX_SB_POSITION: return tr("Scrollback Position", "property-name");
    case TERM_IDX_SB_FETCHPOS: return tr("Downloaded Row", "property-name");
    case TERM_IDX_SB_FETCHNEXT: return tr("Downloading Row", "property-name");
    case TERM_IDX_THROTTLED: return tr("Throttled", "property-name");
    case TERM_IDX_FLAGS: return tr("Emulator Flags", "property-name");
    case TERM_IDX_FL_ANSI: return tr("Ansi Mode", "property-name");
    case TERM_IDX_FL_NEWLINE: return tr("Newline Mode", "property-name");
    case TERM_IDX_FL_APPCUKEYS: return tr("Application Cursor Keys", "property-name");
    case TERM_IDX_FL_APPSCREEN: return tr("Alternate Buffer Active", "property-name");
    case TERM_IDX_FL_APPKEYPAD: return tr("Application Keypad", "property-name");
    case TERM_IDX_FL_REVERSEVIDEO: return tr("Reverse Video", "property-name");
    case TERM_IDX_FL_BLINKSEEN: return tr("Blinking Characters Seen", "property-name");
    case TERM_IDX_FL_HARDSCROLLLOCK: return tr("Hard Scroll Lock", "property-name");
    case TERM_IDX_FL_SOFTSCROLLLOCK: return tr("Soft Scroll Lock", "property-name");
    case TERM_IDX_FL_KEYBOARDLOCK: return tr("Keyboard Action Mode", "property-name");
    case TERM_IDX_FL_SENDRECEIVE: return tr("Send/Receive Mode", "property-name");
    case TERM_IDX_FL_INSERTMODE: return tr("Insert Mode", "property-name");
    case TERM_IDX_FL_LRMMODE: return tr("Left-Right Margin Mode", "property-name");
    case TERM_IDX_FL_ORIGINMODE: return tr("Origin Mode", "property-name");
    case TERM_IDX_FL_SMOOTHSCROLLING: return tr("Smooth Scrolling", "property-name");
    case TERM_IDX_FL_AUTOWRAP: return tr("Autowrap Mode", "property-name");
    case TERM_IDX_FL_REVERSEAUTOWRAP: return tr("Reverse Autowrap Mode", "property-name");
    case TERM_IDX_FL_AUTOREPEAT: return tr("Autorepeat Mode", "property-mode");
    case TERM_IDX_FL_ALLOWCOLUMNCHANGE: return tr("Allow Column Change", "property-name");
    case TERM_IDX_FL_CONTROLS8BIT: return tr("8-bit Controls Mode", "property-name");
    case TERM_IDX_FL_BRACKETEDPASTE: return tr("Bracketed Paste Mode", "property-name");
    case TERM_IDX_FL_CURSORVISIBLE: return tr("Text Cursor Enabled", "property-name");
    case TERM_IDX_FL_X10MOUSE: return tr("X10 Mouse Mode", "property-name");
    case TERM_IDX_FL_NORMALMOUSE: return tr("Basic Mouse Mode", "property-name");
    case TERM_IDX_FL_HIGHLIGHTMOUSE: return tr("Highlight Mouse Mode", "property-name");
    case TERM_IDX_FL_BUTTONEVENTMOUSE: return tr("Button Event Mouse Mode", "property-name");
    case TERM_IDX_FL_ANYEVENTMOUSE: return tr("Any Event Mouse Mode", "property-name");
    case TERM_IDX_FL_FOCUSEVENT: return tr("Send Focus Events", "property-name");
    case TERM_IDX_FL_UTF8EXTMOUSE: return tr("UTF-8 Extended Mouse Events", "property-name");
    case TERM_IDX_FL_SGREXTMOUSE: return tr("SGR Extended Mouse Events", "property-name");
    case TERM_IDX_FL_URXVTEXTMOUSE: return tr("Urxvt Extended Mouse Events", "property-name");
    case TERM_IDX_FL_ALTSCROLLMOUSE: return tr("Alternate Scroll Mode", "property-name");
    case TERM_IDX_IOS_INPUT: return tr("Termios Input Flags", "property-name");
    case TERM_IDX_IOS_IGNBRK: return A("IGNBRK");
    case TERM_IDX_IOS_BRKINT: return A("BRKINT");
    case TERM_IDX_IOS_IGNPAR: return A("IGNPAR");
    case TERM_IDX_IOS_PARMRK: return A("PARMRK");
    case TERM_IDX_IOS_INPCK: return A("INPCK");
    case TERM_IDX_IOS_ISTRIP: return A("ISTRIP");
    case TERM_IDX_IOS_INLCR: return A("INLCR");
    case TERM_IDX_IOS_IGNCR: return A("IGNCR");
    case TERM_IDX_IOS_ICRNL: return A("ICRNL");
    case TERM_IDX_IOS_IUCLC: return A("IUCLC");
    case TERM_IDX_IOS_IXON: return A("IXON");
    case TERM_IDX_IOS_IXANY: return A("IXANY");
    case TERM_IDX_IOS_IXOFF: return A("IXOFF");
    case TERM_IDX_IOS_OUTPUT: return tr("Termios Output Flags", "property-name");
    case TERM_IDX_IOS_OPOST: return A("OPOST");
    case TERM_IDX_IOS_OLCUC: return A("OLCUC");
    case TERM_IDX_IOS_ONLCR: return A("ONLCR");
    case TERM_IDX_IOS_OCRNL: return A("OCRNL");
    case TERM_IDX_IOS_ONOCR: return A("ONOCR");
    case TERM_IDX_IOS_ONLRET: return A("ONLRET");
    case TERM_IDX_IOS_OXTABS: return A("OXTABS");
    case TERM_IDX_IOS_LOCAL: return tr("Termios Local Flags", "property-name");
    case TERM_IDX_IOS_ISIG: return A("ISIG");
    case TERM_IDX_IOS_ICANON: return A("ICANON");
    case TERM_IDX_IOS_ECHO: return A("ECHO");
    case TERM_IDX_IOS_ECHOE: return A("ECHOE");
    case TERM_IDX_IOS_ECHOK: return A("ECHOK");
    case TERM_IDX_IOS_ECHONL: return A("ECHONL");
    case TERM_IDX_IOS_NOFLSH: return A("NOFLSH");
    case TERM_IDX_IOS_TOSTOP: return A("TOSTOP");
    case TERM_IDX_IOS_ECHOCTL: return A("ECHOCTL");
    case TERM_IDX_IOS_ECHOPRT: return A("ECHOPRT");
    case TERM_IDX_IOS_ECHOKE: return A("ECHOKE");
    case TERM_IDX_IOS_FLUSHO: return A("FLUSHO");
    case TERM_IDX_IOS_PENDIN: return A("PENDIN");
    case TERM_IDX_IOS_IEXTEN: return A("IEXTEN");
    case TERM_IDX_IOS_CHARS: return tr("Termios Character Mappings", "property-name");
    case TERM_IDX_IOS_VEOF: return A("VEOF");
    case TERM_IDX_IOS_VEOL: return A("VEOL");
    case TERM_IDX_IOS_VEOL2: return A("VEOL2");
    case TERM_IDX_IOS_VERASE: return A("VERASE");
    case TERM_IDX_IOS_VWERASE: return A("VWERASE");
    case TERM_IDX_IOS_VKILL: return A("VKILL");
    case TERM_IDX_IOS_VREPRINT: return A("VREPRINT");
    case TERM_IDX_IOS_VINTR: return A("VINTR");
    case TERM_IDX_IOS_VQUIT: return A("VQUIT");
    case TERM_IDX_IOS_VSUSP: return A("VSUSP");
    case TERM_IDX_IOS_VDSUSP: return A("VDSUSP");
    case TERM_IDX_IOS_VSTART: return A("VSTART");
    case TERM_IDX_IOS_VSTOP: return A("VSTOP");
    case TERM_IDX_IOS_VLNEXT: return A("VLNEXT");
    case TERM_IDX_IOS_VDISCARD: return A("VDISCARD");
    case TERM_IDX_IOS_VMIN: return A("VMIN");
    case TERM_IDX_IOS_VTIME: return A("VTIME");
    case TERM_IDX_IOS_VSTATUS: return A("VSTATUS");
    case TERM_IDX_PROC_PID: return tr("Process ID", "property-name");
    case TERM_IDX_PROC_CWD: return tr("Working Directory", "property-name");
    case TERM_IDX_PROC_COMM: return tr("Command Name", "property-name");
    case TERM_IDX_PROC_STATUS: return tr("Status", "property-name");
    case TERM_IDX_PROC_OUTCOME: return tr("Outcome", "property-name");
    default: return g_mtstr;
    }
}

//
// Server Model
//
enum ServerRowNumber {
    SERVER_ROW_ID,
    SERVER_ROW_VERSION,
    SERVER_ROW_HOPS,
    SERVER_ROW_PID,
    SERVER_ROW_THROTTLED,
    SERVER_N_ROWS,
};

enum ServerIndexNumber {
    SERVER_IDX_ID,
    SERVER_IDX_VERSION,
    SERVER_IDX_HOPS,
    SERVER_IDX_PID,
    SERVER_IDX_THROTTLED,
    SERVER_N_INDEXES,
};

static const int8_t s_serverChildren[SERVER_N_ROWS] = {
    0, /* id */
    0, /* version */
    0, /* hops */
    0, /* pid */
    0, /* throttled */
};

static const int8_t s_serverCounts[SERVER_N_ROWS] = {
    SERVER_IDX_ID,
    SERVER_IDX_VERSION,
    SERVER_IDX_HOPS,
    SERVER_IDX_PID,
    SERVER_IDX_THROTTLED,
};

// Format: <parentRow> <childRow>
static const int8_t s_serverIndexes[SERVER_N_INDEXES * 2] = {
    SERVER_ROW_ID, -1,
    SERVER_ROW_VERSION, -1,
    SERVER_ROW_HOPS, -1,
    SERVER_ROW_PID, -1,
    SERVER_ROW_THROTTLED, -1,
};

ServerPropModel::ServerPropModel(ServerInstance *server, QWidget *parent, bool setTitle) :
    InfoPropModel(SERVER_N_INDEXES, server, parent),
    m_server(server)
{
    m_nRows = SERVER_N_ROWS;
    m_counts = s_serverCounts;
    m_children = s_serverChildren;
    m_indexes = s_serverIndexes;

    updateData(SERVER_IDX_VERSION, QString::number(server->version()));
    updateData(SERVER_IDX_HOPS, QString::number(server->nHops()));
    updateData(SERVER_IDX_PID, m_server->attributes().value(g_attr_PID));

    connect(server, SIGNAL(throttleChanged(bool)), SLOT(handleThrottleChanged(bool)));
    handleThrottleChanged(server->throttled());

    if (setTitle) {
        connect(m_server->serverInfo(), SIGNAL(shortnameChanged(QString)), SLOT(handleShortnameChanged(QString)));
        parent->setWindowTitle(TR_TITLE2 + server->shortname());
    }
}

void
ServerPropModel::handleShortnameChanged(QString shortname)
{
    static_cast<QWidget*>(QObject::parent())->setWindowTitle(TR_TITLE2 + shortname);
}

void
ServerPropModel::handleThrottleChanged(bool throttled)
{
    QString throttleStr = throttled ? TR_TEXT2 : TR_TEXT1;
    updateData(SERVER_IDX_THROTTLED, throttleStr);
}

QString
ServerPropModel::getRowName(int idx) const
{
    switch (idx) {
    case SERVER_IDX_ID: return tr("Identifier", "property-name");
    case SERVER_IDX_VERSION: return tr("Version", "property-name");
    case SERVER_IDX_HOPS: return tr("Distance (Hops)", "property-name");
    case SERVER_IDX_PID: return tr("PID", "property-name");
    case SERVER_IDX_THROTTLED: return tr("Throttled", "property-name");
    default: return g_mtstr;
    }
}

//
// View
//
InfoPropView::InfoPropView(InfoPropModel *model) :
    m_model(model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);

    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setStretchLastSection(true);
    header()->setSectionsMovable(false);
}

void
InfoPropView::showEvent(QShowEvent *event)
{
    m_model->setVisible(true);
    QTreeView::showEvent(event);
}

void
InfoPropView::hideEvent(QHideEvent *event)
{
    m_model->setVisible(false);
    QTreeView::hideEvent(event);
}
