// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

// Parses "make" output looking for errors and warnings.
// Places a region on the last line of the output that can be used to
// jump to the first error seen.

// Command aliases
// To enable this parser for aliases such as 'm', enter the aliases here:
// var aliases = [ 'make', 'm' ];
var aliases = [ 'make' ];

// Editor launcher
// Specify the launch configuration to run when double-clicking a message.
// In addition to the normal URL and path substitutions, the launch command
// can also contain %l for the line number and %c for the column number.
var editorLauncher = 'Default';

function FileParams(parser, id, icon, path, line, column) {
    var idpath = parser.server + '|' + path;
    var idregion = id + '|' + parser.term;

    var subs = 'l=' + line + '\x1f' + 'c=' + column;
    var action = [ 'OpenFile', editorLauncher, parser.server, path, subs ];
    var uri = 'file://' + parser.host + path;
    var word = icon.charAt(0).toUpperCase() + icon.slice(1);

    this.icon = icon;
    this.action1 = action.join('|');
    this.drag = { 'text/uri-list':uri, 'text/plain':uri };
    this.uri = uri;
    this.tooltip = 'Double-click to open the file using launcher "' +
        editorLauncher + '"';

    this.menu = [
        [ 5, uri, 'Build ' + word ], // Add our menu to the stock file menu
        [ 1, this.action1, 'Launch &Editor', 'edit-item',
          'Open the file at this location using launcher "' +
          editorLauncher + '"' ],
        [ 1, 'CopySemantic|' + idregion, '&Copy Message', 'copy',
          'Copy this diagnostic message to the clipboard' ],
    ];
}

function ErrorParams(parser, id, icon) {
    var idregion = id + '|' + parser.term;

    this.icon = icon;

    this.menu = [
        [ 1, 'CopySemantic|' + idregion, '&Copy Message', 'copy',
          'Copy this diagnostic message to the clipboard' ]
    ];
}

function JumpParams(parser) {
    this.icon = 'go-up';
    this.tooltip = 'Double-click to scroll up to first error';
    this.action1 = 'ScrollSemanticRelative|' + parser.errorid + '|-10';
}

// Get first word of command
var commandre = /^\S+/;
// Match an output line containing a file and error
var fileerrorre = /^(\S+?):(\d+):(\d+:)? (?:error:|fatal error:|undefined reference to|sorry, unimplemented:) /;
// Match an output line containing a file and warning
var filewarnre = /^(\S+?):(\d+):(\d+:)? warning: /;
// Match an output line containing a file and note
var fileinfore = /^(\S+?):(\d+):(\d+:)? (?:note: | *required from here)/;
// Match an output line containing a plain error
var errorre = /(?:error:|undefined reference to|sorry, unimplemented:) /;
// Match an output line containing a plain warning
var warnre = /warning: /;
// Match trailing slashes on a pathname
var slashre = /\/+$/;
// Match a "waiting for unfinished jobs" line
var waitre = /\sWaiting for unfinished jobs\.*$/;

function process(context, text, startrow, endrow) {
    var result;
    var type;

    if ((result = text.match(fileerrorre))) {
        type = 'error';
    } else if ((result = text.match(filewarnre))) {
        type = 'warning';
    } else if ((result = text.match(fileinfore))) {
        type = 'note';
    }

    if (result) {
        var path = result[1];
        if (!path.startsWith('/')) {
            path = this.basedir + path;
        }
        var column = result[3] ? result[3].slice(0, -1) : 1;

        // Get the region id first so we can create the menu using it
        var id = context.nextRegionId();
        var params = new FileParams(this, id, type, path, result[2], column);
        context.createRegion(0, text.length, params);

        if (type == 'error' && !this.haveError) {
            this.haveError = true;
            this.errorid = id;
        }
        this.lastErrorRow = endrow;
        return true;
    }

    if ((result = text.match(errorre))) {
        type = 'error';
    } else if ((result = text.match(warnre))) {
        type = 'warning';
    }

    if (result) {
        var id = context.nextRegionId();
        var params = new ErrorParams(this, id, type);
        context.createRegion(result.index, text.length, params);

        if (type == 'error' && !this.haveError) {
            this.haveError = true;
            this.errorid = id;
        }
        this.lastErrorRow = endrow;
    }

    if (waitre.test(text) && this.haveError) {
        context.createRegion(0, text.length, new JumpParams(this));
    }
    return true;
}

function finish(context, nrows) {
    // the true last line is the empty space before the next prompt,
    // so go up one to get the last line of actual output
    var last = nrows - 2;
    if (this.haveError && this.lastErrorRow < last) {
        context.createRegionAt(last, 0, last + 1, 0, new JumpParams(this));
    }
}

function MakeParser() {
    this.process = process;
    this.finish = finish;
}

function match(context, cmdline, cmddir) {
    var result = cmdline.match(commandre);
    var cmd = result[0];

    for (var alias of aliases) {
        if (cmd == alias) {
            var parser = new MakeParser();
            parser.basedir = cmddir.replace(slashre, '') + '/';
            parser.term = context.getTerminalId();
            parser.server = context.getServerId();
            parser.host = context.getJobAttribute('host');
            return parser;
        }
    }

    return null;
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'MakePlugin';
plugin.pluginDescription = 'Creates semantic regions in make output';
plugin.pluginVersion = '1.0';

// Register as a "Fast Parser"
plugin.registerSemanticParser(1, 'MakeParser', match, 1);
