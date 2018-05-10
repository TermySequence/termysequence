// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/feature.h"
#include "app/v8util.h"
#include "cell.h"

#include <list>
#include <vector>

class Region;
class TermBuffer;
class TermBuffers;
class SemanticFeature;

//
// Standard instance
//
class SemanticParser: public QObject
{
    Q_OBJECT

public:
    enum SpecKey { Icon, Tooltip, Action1, Menu, Drag, Uri, NKeys };

protected:
    TermBuffer *m_buffer;
    Region *m_region;

    PersistentContext m_context;
    PersistentObject m_theirs, m_ours;
    PersistentFunction m_func;

    LocalString m_speckeys[NKeys];

    bool m_populating = true;
    bool m_parsing = false;
    bool m_finished = false;
    int m_timerId = 0;

    index_t m_start, m_end, m_cur;
    size_t m_pos = 0, m_size = 0;

    std::list<std::pair<index_t,index_t>> m_ranges;
    decltype(m_ranges)::iterator m_rangecache;
    std::vector<std::pair<std::string,bool>> m_content;
    std::vector<unsigned> m_breaks;

    QString m_pluginName, m_parserName;

private:
    bool checkStart();
    void start();
    bool process(LocalContext context, LocalObject theirs, LocalObject ours,
                 LocalFunction func);

protected:
    void insertRange(index_t value);
    void stop();

    void timerEvent(QTimerEvent *event);

public:
    SemanticParser(TermBuffer *buffer, Region *output, LocalContext context,
                   LocalObject theirs);

    void destroy();
    virtual bool setRegion();
    virtual bool setRow(index_t index, const CellRow &row);
    unsigned residualPtr(const CellRow &row) const;

    inline bool populating() const { return m_populating; }
    inline bool parsing() const { return m_parsing; }

    inline void setPluginName(const QString &name) { m_pluginName = name; }
    inline void setParserName(const QString &name) { m_parserName = name; }

    static void getSpecKeys(LocalString *keys);
    static void parseRegionSpec(LocalObject spec, Region *r, LocalString *keys);
    regionid_t createRegion(LocalValue spec, unsigned startCol, unsigned endCol);
    regionid_t createRegion(LocalValue spec, size_t startPos, unsigned startCol,
                            size_t endPos, unsigned endCol);
    regionid_t peekRegionId() const;
};

//
// Fast instance
//
class FastSemanticParser final: public SemanticParser
{
private:
    void process(size_t limit);
    bool checkProcess();

public:
    FastSemanticParser(TermBuffer *buffer, Region *output, LocalContext context,
                       LocalObject theirs);

    bool setRegion();
    bool setRow(index_t index, const CellRow &row);
};

//
// Feature
//
class SemanticFeature final: public Feature
{
    friend class SemanticParser;
    friend class FastSemanticParser;

public:
    enum ParserType { StandardSemantic, FastSemantic,
                      NSemanticFeatures };

private:
    PersistentFunction m_matcher;
    int m_type;

public:
    SemanticFeature(Plugin *parent, const QString &name, LocalFunction matcher,
                    ParserType type);

    static SemanticParser* createParser(TermBuffer *buffer, const Region *job, Region *output);
    SemanticParser* getParser(TermBuffer *buffer, Region *output, LocalContext context,
                              LocalObject theirs);

    static void initialize();
    static void teardown();
};
