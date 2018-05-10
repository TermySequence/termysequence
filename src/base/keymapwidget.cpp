// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "keymapwidget.h"
#include "term.h"
#include "scrollport.h"
#include "listener.h"
#include "manager.h"
#include "mainwindow.h"
#include "statuslabel.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/keymap.h"

#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

#define TR_BUTTON1 TL("input-button", "edit", "link")
#define TR_BUTTON2 TL("input-button", "manage", "link")
#define TR_FIELD1 TL("input-field", "Keymap")
#define TR_TEXT1 L("<b>%1</b>:%4 (<a href='#e'>%2</a>|<a href='#m'>%3</a>)")
#define TR_TEXT2 A("<b>") + TL("window-text", "Current Mode") + A("</b>:")
#define TR_TEXT3 A("<b>") + TL("window-text", "Basic Actions") + A("</b>: ")
#define TR_TEXT4 A("<b>") + TL("window-text", "Tool Actions") + A("</b>: ")
#define TR_TEXT5 A("<b>") + TL("window-text", "Additional Action Bindings") + A("</b>:")
#define TR_TEXT6 A("<b>") + TL("window-text", "Literal Bindings") + A("</b>:")
#define TR_TEXT7 A("<b>") + TL("window-text", "Command Mode Action Bindings") + A("</b>:")
#define TR_TEXT8 A("<b>") + TL("window-text", "Command Mode Literal Bindings") + A("</b>:")
#define TR_KEYMAP1 TL("keymap", "Normal", "mode")
#define TR_KEYMAP2 TL("keymap", "Command", "mode")
#define TR_KEYMAP3 TL("keymap", "Selection", "mode")

//
// Selection mode layout
//
class KeymapDirections final: public QWidget
{
private:
    QVector<QWidget*> m_children;
    QSize m_sizeHint;
    QPoint m_point;
    int m_dim;

protected:
    void paintEvent(QPaintEvent *event);

public:
    KeymapDirections(QVector<QWidget*> &&children);
    void relayout();

    QSize sizeHint() const;
};

//
// Contents Widget
//
static QHBoxLayout* hbox()
{
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->setSpacing(0);
    return layout;
}

static QVBoxLayout* vbox()
{
    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->setSpacing(0);
    return layout;
}

#define MAJOR_SEP new QLabel(majorStr)
#define MINOR_SEP new QLabel(minorStr)

#define ADD_DIRECTION(slot) \
    s = A(slot); \
    a = new ActionLabel(manager, Tsqt::SelectMode, s, dpadStr); \
    m_labels.append(a); \
    m_map.insert(s, a); \
    dirvec.append(a);

#define ADD_SELECT(slot) \
    s = A(slot); \
    a = new ActionLabel(manager, Tsqt::SelectMode, s, basicStr.arg(s)); \
    m_labels.append(a); \
    m_map.insert(s, a); \
    sub->addWidget(a)

#define ADD_BASIC(slot) \
    s = A(slot); \
    a = new ActionLabel(manager, 0, s, basicStr.arg(s)); \
    m_labels.append(a); \
    m_map.insert(s, a); \
    sub->addWidget(a)

#define ADD_BASIC_SEP(slot) \
    ADD_BASIC(slot); \
    sub->addWidget(MINOR_SEP)

#define ADD_HSEP(layout) \
    sep = new QFrame; \
    sep->setFrameShape(QFrame::HLine); \
    layout->addWidget(sep);

#define ADD_VSEP(layout) \
    sep = new QFrame; \
    sep->setFrameShape(QFrame::VLine); \
    layout->addWidget(sep);

KeymapWidget::KeymapWidget(TermManager *manager) :
    m_manager(manager)
{
    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)),
            SLOT(handleTermActivated(TermInstance*,TermScrollport*)));

    m_currentStr = TR_TEXT2;
    m_normalStr = TR_KEYMAP1;

    m_header = new QLabel;
    m_header->setTextFormat(Qt::RichText);
    m_header->setContextMenuPolicy(Qt::NoContextMenu);
    m_mode = new QLabel(m_currentStr + m_normalStr);
    m_stack = new QStackedWidget;

    ActionLabel *a;
    QLatin1String s;
    QFrame *sep;
    const QString basicStr = A("<a href='#'>%1</a>:%2");
    const QString dpadStr = A("<a href='#'>%1</a>");
    const QString majorStr = A(" - ");
    const QString minorStr = L(" \u00b7 ");
    auto *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(g_mtmargins);

    QBoxLayout *sub = hbox();
    sub->addWidget(m_header);
    sub->addWidget(MAJOR_SEP);
    ADD_BASIC_SEP("ToggleCommandMode");
    m_toggleCommand = a;
    ADD_BASIC("ToggleSelectionMode");
    m_toggleSelect = a;
    sub->addWidget(MAJOR_SEP);
    sub->addWidget(m_mode);
    sub->addStretch(1);
    mainLayout->addLayout(sub);
    ADD_HSEP(mainLayout);

    auto *basicLayout = new QVBoxLayout;
    basicLayout->setContentsMargins(g_mtmargins);
    auto *innerLayout = vbox();
    sub = hbox();
    sub->addWidget(new QLabel(TR_TEXT3));
    ADD_BASIC_SEP("PreviousTerminal");
    ADD_BASIC_SEP("NextTerminal");
    ADD_BASIC_SEP("NewTerminal");
    ADD_BASIC_SEP("CloseTerminal");
    ADD_BASIC_SEP("NewWindow");
    ADD_BASIC("CloseWindow");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("Copy");
    ADD_BASIC_SEP("Paste");
    ADD_BASIC_SEP("Find");
    ADD_BASIC_SEP("SearchUp");
    ADD_BASIC_SEP("SearchDown");
    ADD_BASIC("SearchReset");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("ScrollPageUp");
    ADD_BASIC_SEP("ScrollPageDown");
    ADD_BASIC_SEP("ScrollToTop");
    ADD_BASIC_SEP("ScrollToBottom");
    ADD_BASIC_SEP("ScrollLineUp");
    ADD_BASIC("ScrollLineDown");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("SplitViewHorizontal");
    ADD_BASIC_SEP("SplitViewVertical");
    ADD_BASIC_SEP("SplitViewClose");
    ADD_BASIC_SEP("PreviousPane");
    ADD_BASIC_SEP("NextPane");
    ADD_BASIC_SEP("SwitchPane|0");
    ADD_BASIC_SEP("SwitchPane|1");
    ADD_BASIC_SEP("SwitchPane|2");
    ADD_BASIC("SwitchPane|3");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("RaiseKeymapTool");
    ADD_BASIC_SEP("RaiseTerminalsTool");
    ADD_BASIC_SEP("RaiseSuggestionsTool");
    ADD_BASIC_SEP("RaiseSearchTool");
    ADD_BASIC_SEP("RaiseFilesTool");
    ADD_BASIC_SEP("RaiseHistoryTool");
    ADD_BASIC_SEP("RaiseAnnotationsTool");
    ADD_BASIC("RaiseTasksTool");
    sub->addStretch(1);
    innerLayout->addLayout(sub);
    basicLayout->addLayout(innerLayout);

    innerLayout = vbox();
    sub = hbox();
    sub->addWidget(new QLabel(TR_TEXT4));
    ADD_BASIC_SEP("SuggestNext");
    ADD_BASIC_SEP("SuggestPrevious");
    ADD_BASIC_SEP("WriteSuggestion");
    ADD_BASIC("RemoveSuggestion");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("WriteSuggestion|0");
    ADD_BASIC_SEP("WriteSuggestion|1");
    ADD_BASIC_SEP("WriteSuggestion|2");
    ADD_BASIC("WriteSuggestion|3");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("ToolPrevious");
    ADD_BASIC_SEP("ToolNext");
    ADD_BASIC_SEP("ToolFirst");
    ADD_BASIC("ToolLast");
    sub->addStretch(1);
    innerLayout->addLayout(sub);

    sub = hbox();
    ADD_BASIC_SEP("ToolAction|0");
    ADD_BASIC_SEP("ToolAction|1");
    ADD_BASIC_SEP("ToolContextMenu");
    ADD_BASIC_SEP("ToolSearch");
    ADD_BASIC("ToolSearchReset");
    sub->addStretch(1);
    innerLayout->addLayout(sub);
    basicLayout->addLayout(innerLayout);

    auto *normalLayout = new QHBoxLayout;
    normalLayout->setContentsMargins(g_mtmargins);
    a_layout[0] = vbox();
    a_layout[0]->addWidget(new QLabel(TR_TEXT5));
    a_layout[0]->addStretch(1);
    normalLayout->addLayout(a_layout[0]);
    ADD_VSEP(normalLayout);
    a_layout[1] = vbox();
    a_layout[1]->addWidget(new QLabel(TR_TEXT6));
    a_layout[1]->addStretch(1);
    normalLayout->addLayout(a_layout[1]);

    normalLayout->addStretch(1);
    basicLayout->addLayout(normalLayout);

    basicLayout->addStretch(1);
    auto *widget = new QWidget;
    widget->setLayout(basicLayout);
    m_stack->addWidget(widget);

    auto *selectLayout = new QHBoxLayout;
    selectLayout->setContentsMargins(g_mtmargins);
    sub = vbox();
    ADD_SELECT("SelectMoveUpLine");
    ADD_SELECT("SelectMoveDownLine");
    ADD_SELECT("SelectMoveBackWord");
    ADD_SELECT("SelectMoveForwardWord");
    ADD_SELECT("SelectWord|0");
    ADD_SELECT("SelectWord|1");
    ADD_SELECT("SelectWord|2");
    ADD_SELECT("SelectWord|3");
    sub->addStretch(1);
    selectLayout->addLayout(sub);
    ADD_VSEP(selectLayout);

    QVector<QWidget*> dirvec;
    ADD_DIRECTION("SelectHandleUpLine");
    ADD_DIRECTION("SelectHandleDownLine");
    ADD_DIRECTION("SelectHandleBackChar");
    ADD_DIRECTION("SelectHandleBackWord");
    ADD_DIRECTION("SelectLine|1");
    ADD_DIRECTION("SelectHandleForwardChar");
    ADD_DIRECTION("SelectHandleForwardWord");
    ADD_DIRECTION("SelectLine|2");
    m_dpad = new KeymapDirections(std::move(dirvec));
    selectLayout->addWidget(m_dpad);
    ADD_VSEP(selectLayout);

    sub = vbox();
    ADD_SELECT("SelectHandle");
    ADD_SELECT("SelectLine");
    ADD_SELECT("WriteSelection");
    ADD_SELECT("Copy");
    sub->addStretch(1);
    selectLayout->addLayout(sub);

    selectLayout->addStretch(1);
    widget = new QWidget;
    widget->setLayout(selectLayout);
    m_stack->addWidget(widget);

    auto *commandLayout = new QHBoxLayout;
    commandLayout->setContentsMargins(g_mtmargins);
    a_layout[2] = vbox();
    a_layout[2]->addWidget(new QLabel(TR_TEXT7));
    a_layout[2]->addStretch(1);
    commandLayout->addLayout(a_layout[2]);
    ADD_VSEP(commandLayout);
    a_layout[3] = vbox();
    a_layout[3]->addWidget(new QLabel(TR_TEXT8));
    a_layout[3]->addStretch(1);
    commandLayout->addLayout(a_layout[3]);

    commandLayout->addStretch(1);
    widget = new QWidget;
    widget->setLayout(commandLayout);
    m_stack->addWidget(widget);

    m_scroll = new QScrollArea;
    m_scroll->setFrameStyle(0);
    m_scroll->setFocusPolicy(Qt::NoFocus);
    m_scroll->setWidget(m_stack);
    m_scroll->setWidgetResizable(true);
    mainLayout->addWidget(m_scroll, 1);
    setLayout(mainLayout);
    setMinimumWidth(1);

    connect(m_header, SIGNAL(linkActivated(const QString&)),
            SLOT(handleLinkActivated(const QString&)));
    connect(m_toggleCommand, &ActionLabel::invokingSlot,
            this, &KeymapWidget::setInvoking);
    connect(m_toggleSelect, &ActionLabel::invokingSlot,
            this, &KeymapWidget::setInvoking);

    m_keymap = g_settings->defaultKeymap();
    m_mocRules = connect(m_keymap, SIGNAL(rulesChanged()), SLOT(handleRulesChanged()));
    m_headerStr = TR_TEXT1.arg(TR_FIELD1, TR_BUTTON1, TR_BUTTON2);
    m_header->setText(m_headerStr.arg(m_keymap->name()));

    m_mocFlags = connect(g_listener, &TermListener::flagsChanged,
                         this, &KeymapWidget::handleFlagsChanged);
}

void
KeymapWidget::doPolish()
{
    const QString spec = L("<font style='background-color:%1;color:%2'>%3</font>")
        .arg(g_global->color(MinorBg).name(), g_global->color(MinorFg).name());
    m_commandStr = spec.arg(TR_KEYMAP2);
    m_selectStr = spec.arg(TR_KEYMAP3);

    handleFlagsChanged(g_listener->flags());
}

void
KeymapWidget::handleTermActivated(TermInstance *term, TermScrollport *scrollport)
{
    disconnect(m_mocFlags);

    if (m_term) {
        disconnect(m_mocKeymap);
    }

    if ((m_term = term)) {
        m_mocKeymap = connect(term, SIGNAL(keymapChanged(const TermKeymap*)),
                              SLOT(handleKeymapChanged(const TermKeymap*)));
        m_mocFlags = connect(scrollport, SIGNAL(flagsChanged(Tsq::TermFlags)),
                             SLOT(handleFlagsChanged(Tsq::TermFlags)));

        handleKeymapChanged(term->keymap());
        handleFlagsChanged(scrollport->flags());
    }
    else {
        m_mocFlags = connect(g_listener, &TermListener::flagsChanged,
                             this, &KeymapWidget::handleFlagsChanged);
        handleKeymapChanged(g_settings->defaultKeymap());
        handleFlagsChanged(g_listener->flags());
    }
}

void
KeymapWidget::processRule(const KeymapRule *rule)
{
    if (rule->conditions & Tsqt::SelectMode)
        return;

    TermShortcut shortcut;
    rule->constructShortcut(shortcut);
    if (m_shortcutMap.contains(shortcut))
        return;

    auto flags = rule->conditions & Tsqt::CommandMode;
    unsigned idx = (!!flags) * 2 | !rule->special;
    auto &count = a_count[idx];
    auto &labels = a_labels[idx];
    auto &layout = a_layout[idx];
    ActionLabel *a;

    if (count == labels.size()) {
        labels.append(a = new ActionLabel(m_manager, flags));
        layout->insertWidget(layout->count() - 1, a);
    } else {
        a = labels[count];
    }

    m_shortcutMap.insert(shortcut, a);

    if (rule->special) {
        a->setAction(rule->outcomeStr, shortcut.expression + A(": <a href='#'>%1</a>"));
        m_additionalMap.insert(rule->outcomeStr, a);
    }
    else if (!shortcut.additional.isEmpty())
        a->setText(shortcut.expression + shortcut.additional + A(": ") + rule->outcomeStr.toHtmlEscaped());
    else
        a->setText(shortcut.expression + A(": ") + rule->outcomeStr.toHtmlEscaped());

    a->show();
    ++count;
}

void
KeymapWidget::processKeymap(const TermKeymap *keymap)
{
    for (const auto &i: *keymap->rules())
        if (!i.second->startsCombo) {
            processRule(i.second);
        } else {
            for (const auto &j: i.second->comboRuleset)
                processRule(j.second);
        }

    if (keymap->parent())
        processKeymap(keymap->parent());
}

void
KeymapWidget::handleRulesChanged()
{
    if (!m_visible) {
        m_changed = true;
        return;
    }

    m_shortcutMap.clear();
    m_additionalMap.clear();
    memset(a_count, 0, sizeof(a_count));

    TermShortcut result;
    for (auto *a: qAsConst(m_labels))
        if (a->updateShortcut(m_keymap, &result))
            m_shortcutMap.insert(result, a);

    processKeymap(m_keymap);

    for (int i = 0; i < 4; ++i)
        for (int k = a_count[i], n = a_layout[i]->count() - 2; k < n; ++k)
            a_labels[i][k]->hide();

    m_dpad->relayout();
}

void
KeymapWidget::handleKeymapChanged(const TermKeymap *keymap)
{
    if (m_keymap != keymap) {
        disconnect(m_mocRules);
        m_keymap = keymap;
        m_mocRules = connect(keymap, SIGNAL(rulesChanged()), SLOT(handleRulesChanged()));
        handleRulesChanged();
        m_header->setText(m_headerStr.arg(keymap->name()));
    }
}

void
KeymapWidget::handleFlagsChanged(Tsq::TermFlags flags)
{
    flags &= Tsqt::CommandMode|Tsqt::SelectMode;
    if (m_flags == flags)
        return;

    Tsq::TermFlags mask;
    mask = g_global->raiseSelect() ? Tsqt::SelectMode : 0;
    mask |= g_global->raiseCommand() ? Tsqt::CommandMode : 0;
    if (mask && !m_invoking) {
        auto *window = m_manager->parent();
        if (mask & flags)
            window->popDockWidget(window->keymapDock());
        else
            window->unpopDockWidget(window->keymapDock());
    }

    m_flags = flags;
    QString modeStr = m_normalStr;
    int tab = 0;

    if (flags == (Tsqt::CommandMode|Tsqt::SelectMode)) {
        modeStr = m_commandStr + '+' + m_selectStr;
        tab = 2;
    }
    else if (flags & Tsqt::CommandMode) {
        modeStr = m_commandStr;
        tab = 2;
    }
    else if (flags & Tsqt::SelectMode) {
        modeStr = m_selectStr;
        tab = 1;
    }

    m_toggleCommand->setFlags(m_keymap, flags);
    m_toggleSelect->setFlags(m_keymap, flags);
    m_mode->setText(m_currentStr + modeStr);

    if (m_stack->currentIndex() != tab) {
        m_stack->setCurrentIndex(tab);
        m_scroll->ensureVisible(0, 0);
    }
}

void
KeymapWidget::handleLinkActivated(const QString &link)
{
    if (link == A("#e")) {
        m_manager->actionEditKeymap(m_keymap->name());
    }
    else if (link == A("#m")) {
        m_manager->actionManageKeymaps();
    }
}

void
KeymapWidget::handleInvocation(const QString &slot)
{
    auto i = m_map.constFind(slot);
    if (i != m_map.cend())
        (*i)->startAnimation();

    auto k = m_additionalMap.constFind(slot);
    auto l = m_additionalMap.cend();

    while (k != l && k.key() == slot)
        (*k++)->startAnimation();
}

void
KeymapWidget::showEvent(QShowEvent *event)
{
    m_visible = true;

    if (m_changed) {
        m_changed = false;
        handleRulesChanged();
    }

    m_mocInvoke = connect(m_manager, SIGNAL(invoking(const QString&)),
                          SLOT(handleInvocation(const QString&)));
}

void
KeymapWidget::hideEvent(QHideEvent *event)
{
    m_visible = false;
    disconnect(m_mocInvoke);
}

void
KeymapWidget::setInvoking(bool invoking)
{
    m_invoking = invoking;
}

//
// Selection mode layout
//
KeymapDirections::KeymapDirections(QVector<QWidget*> &&children) :
    m_children(children)
{
    for (auto i: qAsConst(m_children))
        i->setParent(this);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void
KeymapDirections::relayout()
{
    int height = m_children[0]->sizeHint().height();
    int width = m_children[2]->sizeHint().width();
    width = qMax(width, m_children[3]->sizeHint().width());
    width = qMax(width, m_children[4]->sizeHint().width());
    width = qMax(width, m_children[5]->sizeHint().width());
    width = qMax(width, m_children[6]->sizeHint().width());
    width = qMax(width, m_children[7]->sizeHint().width());

    int m = height;
    m_sizeHint = QSize(2 * width + 5 * height, 5 * height);
    m_dim = height;
    m_point = QPoint(m + width, height);

    int x = (m_sizeHint.width() - m_children[0]->sizeHint().width()) / 2;
    m_children[0]->move(x, 0);
    x = (m_sizeHint.width() - m_children[1]->sizeHint().width()) / 2;
    m_children[1]->move(x, height * 4);
    m_children[2]->move(m + width - m_children[2]->sizeHint().width(), height);
    m_children[3]->move(m + width - m_children[3]->sizeHint().width(), height * 2);
    m_children[4]->move(m + width - m_children[4]->sizeHint().width(), height * 3);
    x = m_sizeHint.width() - width - m;
    m_children[5]->move(x, height);
    m_children[6]->move(x, height * 2);
    m_children[7]->move(x, height * 3);

    updateGeometry();
    update();
}

void
KeymapDirections::paintEvent(QPaintEvent *event)
{
    QPainterPath path;
    path.moveTo(95.0, 20.0);
    path.lineTo(35.0, 50.0);
    path.lineTo(95.0, 80.0);
    path.closeSubpath();

    QPainterPath wordpath(path);
    wordpath.translate(-25, 0);

    QPainterPath linepath;
    linepath.addRect(30.0, 20.0, 20.0, 60.0);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(m_point);
    painter.scale(m_dim / 100.0, m_dim / 100.0);
    painter.fillPath(path, Qt::black);
    painter.translate(0, 100);
    painter.fillPath(path, Qt::black);
    painter.fillPath(wordpath, Qt::black);
    painter.translate(0, 100);
    painter.fillPath(path, Qt::black);
    painter.fillPath(linepath, Qt::black);

    painter.translate(200, -200);
    painter.rotate(90);
    painter.fillPath(wordpath, Qt::black);

    painter.rotate(90);
    painter.translate(-100, -100);
    painter.fillPath(path, Qt::black);
    painter.translate(0, -100);
    painter.fillPath(path, Qt::black);
    painter.fillPath(wordpath, Qt::black);
    painter.translate(0, -100);
    painter.fillPath(path, Qt::black);
    painter.fillPath(linepath, Qt::black);

    painter.translate(200, 0);
    painter.rotate(90);
    painter.fillPath(wordpath, Qt::black);
}

QSize
KeymapDirections::sizeHint() const
{
    return m_sizeHint;
}
