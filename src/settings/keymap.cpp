// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/defmap.h"
#include "app/keys.h"
#include "app/logging.h"
#include "base/manager.h"
#include "base/infoanim.h"
#include "keymap.h"
#include "settings.h"

#include <QRegularExpression>

static QString printSequence(const QByteArray &seq);

static const QRegularExpression s_keyboardLine(L("\\Akeyboard\\s+\"(.*)\"\\z"));
static const QRegularExpression s_inheritLine(L("\\Ainherit\\s+\"(.*)\"\\z"));
static const QRegularExpression s_keyLine(L("\\A(key|digraph)\\s+(.*):\\s+\"(.*)\"\\z"));
static const QRegularExpression s_specLine(L("\\A(key|digraph)\\s+(.*):\\s+(.*)\\z"));
static const QRegularExpression s_featureLine(L("\\A(enable|disable)\\s+(\\w+)\\z"));

TermKeymap::TermKeymap(const QString &name, const QString &path, bool reserved):
    m_mainRuleset(new KeymapRuleset),
    m_currentRuleset(m_mainRuleset),
    m_features(Tsqt::DefaultKeymapFeatures),
    m_path(path),
    m_name(name),
    m_reserved(reserved)
{
}

TermKeymap::~TermKeymap()
{
    delete m_mainRuleset;

    if (m_parent && !g_settings->closing())
        m_parent->putKeymapReference(m_keymapCount);
}

void
TermKeymap::handleParentDestroyed()
{
    m_parent = nullptr;
}

void
TermKeymap::adjustRow(int delta)
{
    m_row += delta;

    if (m_animation)
        m_animation->setData(m_row);
}

InfoAnimation *
TermKeymap::createAnimation()
{
    return m_animation = new InfoAnimation(this, m_row);
}

static inline void
reportUpdate(int row)
{
    if (row != -1)
        emit g_settings->keymapUpdated(row);
}

static const char *
getModifierNum(Tsq::TermFlags state)
{
    switch (state & Tsqt::ModMask) {
    case Tsqt::Shift:
        return "2";
    case Tsqt::Alt:
        return "3";
    case Tsqt::Alt|Tsqt::Shift:
        return "4";
    case Tsqt::Control:
        return "5";
    case Tsqt::Shift|Tsqt::Control:
        return "6";
    case Tsqt::Alt|Tsqt::Control:
        return "7";
    case Tsqt::Shift|Tsqt::Alt|Tsqt::Control:
        return "8";
    case Tsqt::Meta:
        return "9";
    case Tsqt::Meta|Tsqt::Shift:
        return "10";
    case Tsqt::Meta|Tsqt::Alt:
        return "11";
    case Tsqt::Meta|Tsqt::Alt|Tsqt::Shift:
        return "12";
    case Tsqt::Meta|Tsqt::Control:
        return "13";
    case Tsqt::Meta|Tsqt::Control|Tsqt::Shift:
        return "14";
    case Tsqt::Meta|Tsqt::Control|Tsqt::Alt:
        return "15";
    case Tsqt::Meta|Tsqt::Control|Tsqt::Alt|Tsqt::Shift:
        return "16";
    default:
        return "1";
    }
}

void
TermKeymap::reset()
{
    if (m_comboInProgress) {
        m_comboInProgress = false;
        m_currentRuleset = m_mainRuleset;
        emit comboFinished(COMBO_RESET);
    }
}

void
TermKeymap::computeFeatures()
{
    unsigned features;
    features = m_parent ? m_parent->m_features : Tsqt::DefaultKeymapFeatures;
    features |= m_enabledFeatures;
    features &= ~m_disabledFeatures;

    if (m_features != features) {
        m_features = features;
        emit featuresChanged();
    }
}

QByteArray
TermKeymap::translateCombo(TermManager *manager, const QKeyEvent *event, Tsq::TermFlags state)
{
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    QByteArray result;

    auto p = m_currentRuleset->equal_range(key);
    auto i = p.first, j = p.second;

    // reset
    m_comboInProgress = false;
    m_currentRuleset = m_mainRuleset;

    // Esc to cancel
    if (isEscape(key)) {
        qCDebug(lcKeymap) << "Combo cancelled";
        emit comboFinished(COMBO_CANCEL);
        return result;
    }

    if (i != j) {
        state |= modifiers;
        if (state & Tsqt::ModMask)
            state |= Tsqt::AnyMod;

        do {
            const KeymapRule *rule = i->second;

            if ((state ^ rule->conditions) & rule->mask) {
                ++i;
                continue;
            }

            result = rule->outcome;
            emit comboFinished(COMBO_SUCCESS);

            if (rule->special) {
                qCDebug(lcKeymap) << "translated" << key << "special" << printSequence(result);
                manager->invokeSlot(result, true);
                result.clear();
            }
            else {
                qCDebug(lcKeymap) << "translated" << key << "to" << printSequence(result);
                if (rule->conditions & Tsqt::AnyMod && result.contains('*'))
                    result.replace('*', getModifierNum(state));
            }

            return result;
        } while (i != j);
    }

    // At this point, invalid combo received
    qCDebug(lcKeymap) << "Bad combo";
    emit comboFinished(COMBO_FAIL);
    return result;
}

QByteArray
TermKeymap::translate(TermManager *manager, const QKeyEvent *event, Tsq::TermFlags state)
{
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    QByteArray result;

    // Ignore pure modifier presses
    if (isModifier(key))
        return result;
    // Separate processing for digraphs
    if (m_comboInProgress)
        return translateCombo(manager, event, state);

    auto p = m_currentRuleset->equal_range(key);
    auto i = p.first, j = p.second;

    if (i != j) {
        state |= modifiers;
        if (state & Tsqt::ModMask)
            state |= Tsqt::AnyMod;

        do {
            KeymapRule *rule = i->second;

            if ((state ^ rule->conditions) & rule->mask) {
                ++i;
                continue;
            }

            result = rule->outcome;

            if (rule->startsCombo) {
                qCDebug(lcKeymap) << "starting combo on" << key;
                result.clear();
                m_currentRuleset = &rule->comboRuleset;
                m_comboInProgress = true;
                manager->reportComboStarted(this);
            }
            else if (rule->special) {
                qCDebug(lcKeymap) << "translated" << key << "special" << printSequence(result);
                manager->invokeSlot(result, true);
                result.clear();
            }
            else {
                qCDebug(lcKeymap) << "translated" << key << "to" << printSequence(result);
                if (rule->conditions & Tsqt::AnyMod && result.contains('*'))
                    result.replace('*', getModifierNum(state));
            }

            return result;
        } while (i != j);
    }

    if (m_parent)
        return m_parent->translate(manager, event, state);

    result = event->text().toUtf8();

    if (modifiers & m_features && !result.isEmpty())
        result.prepend('\x1b');

    qCDebug(lcKeymap) << "keypress" << key << "is" << printSequence(result);
    return result;
}

void
TermKeymap::addShortcut(KeymapRule *rule, const TermShortcut *parent)
{
    // Update the rule's expression if necessary
    if (parent) {
        rule->expressionStr.prepend(parent->expression + ',');
    }

    if (rule->special) {
        TermShortcut shortcut;
        rule->constructShortcut(shortcut);

        qCDebug(lcKeymap) << "Adding shortcut" << pr(rule->outcomeStr) << "->"
                          << pr(shortcut.expression) << "(" << pr(shortcut.additional) << ")";

        m_shortcuts.insert(rule->outcomeStr, shortcut);
    }
}

bool
TermKeymap::parseRule(const QStringList &parts, bool special, int lineno)
{
    KeymapRule *rulep = new KeymapRule;
    KeymapRule &rule = *rulep;
    QString failreason;

    rule.parse(parts, special);

    if (!rule.continuesCombo) {
        m_comboInProgress = false;
        m_currentRuleset = m_mainRuleset;
    }
    if (rule.continuesCombo && !m_comboInProgress) {
        failreason = A("Unexpected digraph definition");
        goto bad;
    }
    if (!g_keyNames.contains(rule.keyName)) {
        failreason = A("Unknown key name: ") + rule.keyName;
        goto bad;
    }

    rule.key = g_keyNames[rule.keyName];
    rule.populateStrings();

    // Starting combo?
    if (rule.startsCombo) {
        if (m_comboInProgress) {
            failreason = A("Nested digraph");
            goto bad;
        }
        rule.constructShortcut(m_digraphShortcut);
    }
    // Ignore unknown actions
    else if (rule.special && !TermManager::validateSlot(rule.outcome)) {
        qCWarning(lcKeymap, "%s: Line %d: Unknown action \"%s\"", pr(m_path), lineno, rule.outcome.data());
        delete rulep;
        return true;
    }
    // Set shortcut
    else {
        addShortcut(rulep, rule.continuesCombo ? &m_digraphShortcut : nullptr);
    }

    m_currentRuleset->insert(rulep);
    if (rule.startsCombo) {
        m_comboInProgress = true;
        m_currentRuleset = &rule.comboRuleset;
    }

    qCDebug(lcKeymap) << m_name << rule.conditions << rule.mask << rule.special << rule.outcomeStr << rule.keyName;
    ++m_size;
    return true;
bad:
    qCWarning(lcKeymap, "%s: Parse error on line %d: %s", pr(m_path), lineno, pr(failreason));
    delete rulep;
    return false;
}

void
TermKeymap::reparent(TermKeymap *parent)
{
    if (m_parent != parent) {
        if (m_parent) {
            m_parent->disconnect(this);
            m_parent->putKeymapReference(m_keymapCount);
        }

        m_parent = parent;

        if (m_parent) {
            m_parent->takeKeymapReference(m_keymapCount);
            connect(m_parent, SIGNAL(featuresChanged()), SLOT(computeFeatures()));
            connect(m_parent, SIGNAL(destroyed()), SLOT(handleParentDestroyed()));
        }

        computeFeatures();

        reportUpdate(m_row);
        emit rulesChanged();
    }
}

void
TermKeymap::parseFeature(const QStringList &parts, int lineno)
{
    auto &var = parts[1] == A("enable") ? m_enabledFeatures : m_disabledFeatures;

    if (parts[2] == A("EscapeOnAlt"))
        var |= Tsqt::Alt;
    else if (parts[2] == A("EscapeOnMeta"))
        var |= Tsqt::Meta;
    else
        qCWarning(lcKeymap, "%s: Unrecognized feature \"%s\" on line %d", pr(m_path), pr(parts[2]), lineno);
}

bool
TermKeymap::parseInheritance(const QString &target, int lineno)
{
    if (m_reserved) {
        qCWarning(lcKeymap, "%s: Error on line %d: Default keymap cannot inherit from another keymap", pr(m_path), lineno);
        return false;
    }

    TermKeymap *parent = g_settings->keymap(target);
    // Cycles stopped in load()
    parent->activate();

    if (parent->name() != target)
        qCWarning(lcKeymap, "%s: Error on line %d: Keymap %s not found, inheriting Default keymap", pr(m_path), lineno, pr(target));

    for (const TermKeymap *tmp = parent; tmp; tmp = tmp->parent())
        if (tmp == this) {
            qCWarning(lcKeymap, "%s: Error on line %d: Keymap inherits itself", pr(m_path), lineno);
            return false;
        }

    reparent(parent);
    return true;
}

bool
TermKeymap::parseFile(QFile &file)
{
    m_size = 0;
    m_shortcuts.clear();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcKeymap, "Failed to open %s: %s", pr(m_path), pr(file.errorString()));
        return false;
    }

    QTextStream in(&file);
    int lineno = 0;
    bool rc = false;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        ++lineno;

        if (line.isEmpty())
            continue;
        if (line.startsWith('#'))
            continue;

        QRegularExpressionMatch match;

        if ((match = s_keyLine.match(line)).hasMatch()) {
            if (!parseRule(match.capturedTexts(), false, lineno))
                goto err;
        }
        else if ((match = s_specLine.match(line)).hasMatch()) {
            if (!parseRule(match.capturedTexts(), true, lineno))
                goto err;
        }
        else if ((match = s_inheritLine.match(line)).hasMatch()) {
            if (!parseInheritance(match.captured(1), lineno))
                goto err;
        }
        else if ((match = s_featureLine.match(line)).hasMatch()) {
            parseFeature(match.capturedTexts(), lineno);
        }
        else if ((match = s_keyboardLine.match(line)).hasMatch()) {
            m_description = match.captured(1);
        }
        else {
            qCWarning(lcKeymap, "%s: Parse error on line %d: Unrecognized rule", pr(m_path), lineno);
            goto err;
        }
    }

    computeFeatures();
    rc = true;
err:
    reset();
    return rc;
}

void
TermKeymap::resetToDefaults()
{
    m_mainRuleset->clear();
    m_shortcuts.clear();

    reparent(nullptr);
    m_enabledFeatures = m_disabledFeatures = 0;
    computeFeatures();

    // NOTE: No digraphs here
    for (const KeymapRule *cur = g_defaultKeymap; cur->key; ++cur) {
        KeymapRule *rule = new KeymapRule(*cur);
        m_mainRuleset->insert(rule);
        rule->populateStrings();
        addShortcut(rule, nullptr);
    }

    m_size = m_mainRuleset->size();
    reset();
}

void
TermKeymap::setFeatures(unsigned enabled, unsigned disabled)
{
    m_enabledFeatures = enabled;
    m_disabledFeatures = disabled;
    computeFeatures();
}

void
TermKeymap::setRules(const KeymapRulelist *rules)
{
    m_mainRuleset->clear();
    m_shortcuts.clear();

    int size = 0;

    for (auto i: *rules) {
        KeymapRule *rule = new KeymapRule(*i);
        m_mainRuleset->insert(rule);

        if (rule->startsCombo) {
            rule->constructShortcut(m_digraphShortcut);

            for (auto k: qAsConst(i->comboRulelist)) {
                KeymapRule *child = new KeymapRule(*k);
                rule->comboRuleset.insert(child);
                addShortcut(child, &m_digraphShortcut);

                ++size;
            }
        }
        else {
            addShortcut(rule, nullptr);
        }

        ++size;
    }

    m_size = size;
    reset();
    reportUpdate(m_row);
    emit rulesChanged();
}

void
TermKeymap::load()
{
    if (m_size) {
        m_mainRuleset->clear();
        m_shortcuts.clear();
        m_size = 0;
        reset();
    }

    m_enabledFeatures = m_disabledFeatures = 0;

    QFile file(m_path.toUtf8().constData());

    if (file.exists()) {
        if ((m_error = !parseFile(file))) {
            if (m_reserved) {
                qCWarning(lcKeymap, "Falling back to compiled-in values for keymap %s", pr(m_name));
                resetToDefaults();
            } else {
                qCWarning(lcKeymap, "Falling back to Default keymap for keymap %s", pr(m_name));
                g_settings->defaultKeymap()->copy(this);
            }
        }
    }
    else if (m_reserved) {
        resetToDefaults();
    }
    else {
        g_settings->defaultKeymap()->copy(this);
    }
}

static void
writeRule(QFile &file, const KeymapRule *rule)
{
    QByteArray line;
    rule->write(line);
    file.write(line);

    if (rule->startsCombo)
        for (auto &i: rule->comboRuleset)
            writeRule(file, i.second);
}

void
TermKeymap::save()
{
    QFile file(m_path.toUtf8().constData());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcKeymap, "Failed to open %s for writing: %s", pr(m_path), pr(file.errorString()));
        return;
    }

    if (!m_description.isEmpty())
        file.write(L("keyboard \"%1\"\n\n").arg(m_description).toUtf8());
    if (m_parent)
        file.write(L("inherit \"%1\"\n\n").arg(m_parent->name()).toUtf8());

    if (m_disabledFeatures & Tsqt::Alt)
        file.write(B("disable EscapeOnAlt\n"));
    else if (m_enabledFeatures & Tsqt::Alt)
        file.write(B("enable EscapeOnAlt\n"));

    if (m_disabledFeatures & Tsqt::Meta)
        file.write(B("disable EscapeOnMeta\n"));
    else if (m_enabledFeatures & Tsqt::Meta)
        file.write(B("enable EscapeOnMeta\n"));

    for (auto &i: *m_mainRuleset)
        writeRule(file, i.second);

    m_loaded = true;
}

void
TermKeymap::copy(TermKeymap *other) const
{
    *(other->m_mainRuleset) = *m_mainRuleset;
    other->m_description = m_description;
    other->m_size = m_size;

    if (other->m_parent)
        other->m_parent->putKeymapReference(other->m_keymapCount);

    other->m_parent = m_parent;

    if (m_parent)
        m_parent->takeKeymapReference(other->m_keymapCount);

    other->m_enabledFeatures = m_enabledFeatures;
    other->m_disabledFeatures = m_disabledFeatures;
    other->computeFeatures();
}

void
TermKeymap::reload()
{
    load();
    reportUpdate(m_row);
    emit rulesReloaded();
    emit rulesChanged();
}

void
TermKeymap::activate()
{
    if (!m_loaded) {
        m_loaded = true; // stops inherit cycles
        load();
    }
}

bool
TermKeymap::lookupShortcut(const QString &slot, const Tsq::TermFlags flags,
                           TermShortcut *result) const
{
    auto i = m_shortcuts.constFind(slot), j = m_shortcuts.cend();
    bool found = false;

    while (i != j && i.key() == slot) {
        const TermShortcut &shortcut = *i++;

        if (((flags ^ shortcut.conditions) & shortcut.mask) == 0) {
            *result = shortcut;
            found = true;
        }
    }

    return found || (m_parent && m_parent->lookupShortcut(slot, flags, result));
}

static QString
printSequence(const QByteArray &seq)
{
    QString s;

    for (int i = 0; i < seq.size(); ++i) {
        unsigned val = seq.at(i);
        if (val > 0x20 && val <= 0x7e) {
            s.push_back(val);
        } else {
            s.push_back(L("(%1)").arg(val));
        }
    }

    return s;
}

void
TermKeymap::takeKeymapReference(int count)
{
    m_keymapCount += count;

    if (m_parent)
        m_parent->takeKeymapReference(count);
    else
        reportUpdate(m_row);
}

void
TermKeymap::putKeymapReference(int count)
{
    m_keymapCount -= count;

    if (m_parent)
        m_parent->putKeymapReference(count);
    else
        reportUpdate(m_row);
}

void
TermKeymap::takeProfileReference()
{
    ++m_profileCount;
    reportUpdate(m_row);
}

void
TermKeymap::putProfileReference()
{
    --m_profileCount;
    reportUpdate(m_row);
}

void
TermKeymap::setDescription(const QString &description)
{
    m_description = description;
    reportUpdate(m_row);
}
