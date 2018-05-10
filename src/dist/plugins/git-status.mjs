// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

// Command alias
// To enable this parser for an alias such as 'git s' or 's', enter the alias here:
// var alias = 'git s';
var alias = '';

function GitParams(parser, dir, file) {
    var path = dir + file;

    this.icon = 'git';
    this.tooltip = 'file://' + parser.host + path;
    this.action1 = [ 'OpenFile', '', parser.server, path ].join('|');
    this.drag = { 'text/uri-list':this.tooltip, 'text/plain':this.tooltip };
    this.uri = this.tooltip;

    // Add our menu to the stock file menu
    this.menu = [[ 5, this.uri, "Git" ]];

    var run = 'RunCommand|' + parser.server + "|";
    var cmdspec;
    var item;

    switch (parser.state) {
    case 1: // staged
        cmdspec = [ 'git', 'git', 'reset', '--', file ];
        this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                         'Reset File', 'remove-item',
                         'Remove this file from the index in git' ],
                       [ 0 ]);
        break;
    case 2: // modified
    case 3: // conflict
    case 4: // untracked
        cmdspec = [ 'git', 'git', 'add', '--', file ];
        this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                         'Add File', 'insert-item',
                         'Add this file to the index' ]);
        cmdspec = [ 'git', 'git', 'checkout', '--', file ];
        this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                         'Checkout File', 'clean',
                         'Discard unstaged changes to this file' ],
                       [ 2, 'This will permanently discard unstaged changes to '
                         + file + '. Proceed?' ],
                       [ 0 ]);
        break;
    case 5: // ignored
        this.icon = 'file';
        break;
    }

    cmdspec = [ 'git', 'git', 'add', '--', '.' ];
    this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                     'Add Directory', 'insert-item',
                     'Add all changes in this directory to the index' ]);
    cmdspec = [ 'git', 'git', 'reset', '--', '.' ];
    this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                     'Reset Directory', 'remove-item',
                     'Remove all changes in this directory from the index' ],
                   [ 0 ]);

    cmdspec = [ 'git', 'git', 'checkout', '--', '.' ];
    this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                     'Checkout Directory', 'clean',
                     'Discard all unstaged changes in this directory' ],
                   [ 2, 'This will permanently discard all unstaged changes in '
                     + dir + '. Proceed?' ]);
    cmdspec = [ 'git', 'git', 'reset', '--hard' ];
    this.menu.push([ 1, run + cmdspec.join('\x1f') + '|' + dir,
                     'Reset Hard', 'destroy',
                     'Discard all staged and unstaged changes' ],
                   [ 2, 'This will permanently discard ALL uncommitted changes. '
                     + 'Proceed?' ]);
}

// Split line into words
var splitre = /\S+/g;
var commandre = /^git status\b/;
var stagedre = /^Changes to be committed:/;
var modifiedre = /^Changes not staged for commit:/;
var conflictre = /^Unmerged paths:/;
var untrackedre = /^Untracked files:/;
var ignoredre = /^Ignored files:/;
// Match an output line containing a filename
var filere = /^        (?:[\w ]+:   +(?:\S+ -> )?)?(\S+)$/;
// Match trailing slashes on a pathname
var slashre = /\/+$/;

function process(context, text) {
    var result = text.match(filere);

    if (result && this.state) {
        var file = result[1];

        // Path cannot contain separator characters
        if (file.indexOf('|') + file.indexOf('\x1f') > -2)
            return true;

        context.createRegion(text.length - file.length, text.length,
                             new GitParams(this, this.basedir, file));
    } else if (stagedre.test(text)) {
        this.state = 1; // staged
    } else if (modifiedre.test(text)) {
        this.state = 2; // modified
    } else if (conflictre.test(text)) {
        this.state = 3; // conflict
    } else if (untrackedre.test(text)) {
        this.state = 4; // untracked
    } else if (ignoredre.test(text)) {
        this.state = 5; // ignored
    }
    return true;
}

function GitStatusParser() {
    this.start = function(context, exitcode) { return exitcode == 0 };
    this.process = process;
    this.state = 0; // initial state
}

function match(context, cmdline, cmddir) {
    if (!commandre.test(cmdline))
        if (!alias || cmdline != alias)
            return null;

    // Path cannot contain separator characters
    if (cmddir.indexOf('|') + cmddir.indexOf('\x1f') > -2)
        return null;

    var result;
    while (result = splitre.exec(cmdline)) {
        var word = result[0];
        // TODO support some of these other output formats
        if (word.startsWith('-')) {
            if (word.includes('s') ||
                word.includes('porcelain') ||
                word.includes('v') ||
                word.includes('z') ||
                word.includes('--column'))
                return null;
        }
    }

    var parser = new GitStatusParser();
    parser.basedir = cmddir.replace(slashre, '') + '/';
    parser.server = context.getServerId();
    parser.host = context.getJobAttribute('host');
    return parser;
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'GitStatusPlugin';
plugin.pluginDescription = 'Creates semantic regions in git status output';
plugin.pluginVersion = '1.0';

plugin.registerSemanticParser(1, 'GitStatusParser', match);
