// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "dircolors.h"

#include "app/defdircolors.hpp"

static CellAttributes32
parseColorSpec(const QString &spec)
{
    CellAttributes32 result;
    QStringList list = spec.split(';');

    for (int i = 0; i < list.size(); ++i) {
        int num = list[i].toInt();

        if (num == 1) {
            result.flags |= Tsq::Bold;
        }
        else if (num == 4) {
            result.flags |= Tsq::Underline;
        }
        else if (num == 5) {
            result.flags |= Tsq::Blink;
        }
        else if (num == 7) {
            result.flags |= Tsq::Inverse;
        }
        else if (num == 8) {
            result.flags |= Tsq::Invisible;
        }
        else if (num >= 30 && num < 38) {
            result.flags |= (Tsq::Fg|Tsq::FgIndex);
            result.fg = num - 30;
        }
        else if (num == 38) {
            if (i >= list.size() - 2)
                break;
            if (list[++i].toInt() != 5)
                break;

            num = list[++i].toInt();
            if (num < 0 || num > 255)
                break;

            result.flags |= (Tsq::Fg|Tsq::FgIndex);
            result.fg = num;
        }
        else if (num >= 40 && num < 48) {
            result.flags |= (Tsq::Bg|Tsq::BgIndex);
            result.bg = num - 40;
        }
        else if (num == 48) {
            if (i >= list.size() - 2)
                break;
            if (list[++i].toInt() != 5)
                break;

            num = list[++i].toInt();
            if (num < 0 || num > 255)
                break;

            result.flags |= (Tsq::Bg|Tsq::BgIndex);
            result.bg = num;
        }
        else {
            break;
        }
    }
    return result;
}

static QString
getColorSpec(const CellAttributes32 &attr)
{
    QStringList list;

    if (attr.flags & Tsq::Bold)
        list += C('1');
    if (attr.flags & Tsq::Underline)
        list += C('4');
    if (attr.flags & Tsq::Blink)
        list += C('5');
    if (attr.flags & Tsq::Inverse)
        list += C('7');
    if (attr.flags & Tsq::Invisible)
        list += C('8');
    if (attr.flags & Tsq::Fg) {
        list += (attr.fg < 8) ?
            QString::number(30 + attr.fg) :
            A("38;5;") + QString::number(attr.fg);
    }
    if (attr.flags & Tsq::Bg) {
        list += (attr.bg < 8) ?
            QString::number(40 + attr.bg) :
            A("48;5;") + QString::number(attr.bg);
    }

    QString result = C('=');
    result += list.join(';');
    result += ':';
    return result;
}

inline void
Dircolors::setDefaults1()
{
    for (unsigned i = 0; i < ARRAY_SIZE(s_dirVars); ++i)
        m_extensions.insert('\0' + s_dirVars[i].id, s_dirVars[i].attr);
    for (unsigned i = 0; i < ARRAY_SIZE(s_dirCats); ++i)
        m_categories[s_dirCats[i].category] = s_dirCats[i].attr;
}

inline void
Dircolors::setDefaults2()
{
    for (unsigned i = 0; i < ARRAY_SIZE(s_dirExts); ++i) {
        QString var = '\0' + s_dirExts[i].spec;
        if (!m_extensions.contains(s_dirExts[i].id))
            m_extensions.insert(s_dirExts[i].id, m_extensions[var]);
    }
}

std::pair<bool,bool>
Dircolors::parse(const QString &spec)
{
    m_spec = spec;
    m_env.clear();
    bool inherits = false;
    int pos = 0;

    if (m_spec.size() > 1 && !m_spec.endsWith(':')) {
        m_spec.append(':');
    }
    if (m_spec.isEmpty() || m_spec[0] == '+') {
        inherits = true;
        pos = !m_spec.isEmpty();
        setDefaults1();
    }

    auto j = s_dcHash.cend();
    auto iter = s_dcRe.globalMatch(m_spec, pos, QRegularExpression::NormalMatch,
                                   QRegularExpression::AnchoredMatchOption);

    while (iter.hasNext()) {
        auto match = iter.next();
        pos = match.capturedEnd();
        QString id = match.captured(1);
        QString spec = match.captured(2);
        CellAttributes32 attr;

        if (spec.at(0).isDigit()) {
            attr = parseColorSpec(spec);
        }
        else if (spec != A("target")) {
            attr = m_extensions.value('\0' + spec);
        }
        else if (id == A("ln")) {
            attr.flags = UseLinkTarget;
        }
        else {
            continue;
        }

        if (id.startsWith('$')) {
            id[0] = '\0';
            m_extensions[id] = attr;
            continue;
        }

        auto i = s_dcHash.constFind(id);
        if (i != j) {
            attr.flags |= CategorySet;
            m_categories[*i] = attr;
        } else if (id.startsWith(A("*."))) {
            id.remove(0, 2);
            m_extensions[id] = attr;
        }
    }

    if (inherits)
        setDefaults2();

    return std::make_pair(inherits, pos == m_spec.size());
}

Dircolors::Dircolors() :
    m_categories{}
{
    setDefaults1();
    setDefaults2();
}

Dircolors::Dircolors(const QString &spec, bool *parseOk) :
    m_categories{}
{
    auto ret = parse(spec);
    if (parseOk)
        *parseOk = ret.second;
}

const QString &
Dircolors::envStr() const
{
    if (m_env.isEmpty()) {
        m_env = A("+LS_COLORS=");

        for (unsigned i = 0; i < NCategories; ++i) {
            if (m_categories[i].flags & CategorySet && s_dcNames[i]) {
                m_env += s_dcNames[i];

                if (m_categories[i].flags == UseLinkTarget)
                    m_env += A("=target:");
                else
                    m_env += getColorSpec(m_categories[i]);
            }
        }
        for (auto i = m_extensions.begin(), j = m_extensions.end(); i != j; ++i) {
            if (i.key().at(0) != C(0)) {
                m_env += A("*.") + i.key() + getColorSpec(*i);
            }
        }
    }
    return m_env;
}

void
DircolorsDisplay::putEnt(Entry &ent, QVector<Entry> &ilist, QVector<Entry> &list)
{
    for (auto k = ilist.begin(), l = ilist.end(); k != l; ++k)
        if (ent.id == k->id) {
            ilist.erase(k);
            break;
        }
    for (auto k = list.begin(), l = list.end(); k != l; ++k)
        if (ent.id == k->id) {
            *k = std::move(ent);
            return;
        }
    list.append(std::move(ent));
}

void
DircolorsDisplay::parseForDisplay(const QString &spec, QVector<Entry> &cats)
{
    memset(m_categories, 0, sizeof(m_categories));
    m_extensions.clear();

    QVector<Entry> icats, iexts, exts, ivars, vars;
    auto rc = parse(spec);
    int pos = 0;

    if (rc.first) {
        pos = !m_spec.isEmpty();

        for (const auto *ptr = s_dirCats;
             (ptr - s_dirCats) < ARRAY_SIZE(s_dirCats); ++ptr)
        {
            Entry ent = { ptr->id, ptr->spec, -1, -1, ptr->attr, 1, ptr->category };
            icats.append(std::move(ent));
        }
        for (const auto *ptr = s_dirVars;
             (ptr - s_dirVars) < ARRAY_SIZE(s_dirVars); ++ptr)
        {
            Entry ent = { ptr->id, ptr->spec, -1, -1, ptr->attr, 2 };
            ivars.append(std::move(ent));
        }
        for (const auto *ptr = s_dirExts;
             (ptr - s_dirExts) < ARRAY_SIZE(s_dirExts); ++ptr)
        {
            QString var = '\0' + ptr->spec;
            Entry ent = { ptr->id, ptr->spec, -1, -1, m_extensions[var] };
            iexts.append(std::move(ent));
        }
    }

    if (!rc.second) {
        icats.append(ivars);
        icats.append(iexts);
        cats = std::move(icats);
        return;
    }

    cats.clear();
    auto j = s_dcHash.cend();
    auto iter = s_dcRe.globalMatch(m_spec, pos, QRegularExpression::NormalMatch,
                                   QRegularExpression::AnchoredMatchOption);

    while (iter.hasNext()) {
        auto match = iter.next();
        Entry ent = { match.captured(1), match.captured(2),
                      match.capturedStart(), match.capturedEnd() };

        if (ent.id.startsWith('$')) {
            ent.type = 2;
            ent.id[0] = C(0);
            ent.attr = m_extensions[ent.id];
            ent.id.remove(0, 1);
        }
        else {
            auto i = s_dcHash.constFind(ent.id);
            if (i != j) {
                ent.type = 1;
                ent.category = *i;
                ent.attr = m_categories[*i];
            } else if (ent.id.startsWith(A("*."))) {
                ent.id.remove(0, 2);
                ent.attr = m_extensions[ent.id];
            } else {
                continue;
            }
        }

        switch (ent.type) {
        case 1:
            putEnt(ent, icats, cats);
            break;
        case 2:
            putEnt(ent, ivars, vars);
            break;
        default:
            putEnt(ent, iexts, exts);
            break;
        }
    }

    cats.append(vars);
    cats.append(exts);
    cats.append(icats);
    cats.append(ivars);
    cats.append(iexts);
}
