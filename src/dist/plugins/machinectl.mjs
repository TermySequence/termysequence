// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

// Command alias
// To enable this parser for an alias, enter the alias here:
// var list_alias = 'mcl';
var list_alias = '';
var list_images_alias = '';

function MachineParams(parser, name) {
    this.icon = 'container';
    this.tooltip = 'Double-click to open connection';
    this.action1 = [ 'NewConnection', '9', name, parser.server ].join('|');
    this.drag = { 'text/plain':name };

    var cmdspec = [ 'machinectl', 'machinectl', 'login', name ];
    var run = [ 'CommandTerminal', '', parser.server, cmdspec.join('\x1f') ];

    this.menu = [
        [ 1, this.action1, '&Connect', 'connection-launch',
          'Open a connection to this container' ],
        [ 1, run.join('|'), '&Login', 'new-terminal',
          'Login to this container in a new terminal' ],
        [ 0 ]
    ];

    cmdspec[2] = 'status';
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Status', 'inspect-item',
                     'Get container status'], [ 0 ]);

    cmdspec[2] = 'poweroff';
    run = [ 'RunCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Poweroff', 'shutdown',
                     'Power off the container']);

    cmdspec[2] = 'reboot';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Reboot', 'reboot',
                     'Reboot the container']);

    cmdspec[2] = 'terminate';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Terminate', 'destroy',
                     'Terminate the container']);
    this.menu.push([ 2, 'This will halt container ' + name +
                     ' without shutting down cleanly. Proceed?' ]);
}

function ImageParams(parser, name) {
    var cmdspec = [ 'machinectl', 'machinectl', 'start', name ];
    var run = [ 'RunCommand', parser.server, cmdspec.join('\x1f') ];

    this.icon = 'container';
    this.tooltip = 'Double-click to start container';
    this.action1 = run.join('|');
    this.drag = { 'text/plain':name };

    this.menu = [
        [ 1, this.action1, '&Start Container', 'resume',
          'Start a container using this image' ]
    ];
}

// Get first word of line
var firstre = /^\S+\b/g;

function list_process(context, text) {
    if (text == '') {
        return false; // stop on blank line
    }
    var result = text.match(firstre);
    if (result) {
        if (result[0] == 'MACHINE') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new MachineParams(this, result[0]));
        }
    }
    return true;
}

function image_process(context, text) {
    if (text == '') {
        return false; // stop on blank line
    }
    var result = text.match(firstre);
    if (result) {
        if (result[0] == 'NAME') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new ImageParams(this, result[0]));
        }
    }
    return true;
}

function Parser(process_func) {
    this.start = function(context, exitcode) { return exitcode == 0 };
    this.process = process_func;
    this.state = false;
}

function list_match(context, cmdline) {
    if (cmdline != 'machinectl list')
        if (!list_alias || cmdline != list_alias)
            return null;

    var parser = new Parser(list_process);
    parser.server = context.getServerId();
    return parser;
}

function image_match(context, cmdline) {
    if (cmdline != 'machinectl list-images')
        if (!list_images_alias || cmdline != list_images_alias)
            return null;

    var parser = new Parser(image_process);
    parser.server = context.getServerId();
    return parser;
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'MachinectlPlugin';
plugin.pluginDescription = 'Creates semantic regions in machinectl output';
plugin.pluginVersion = '1.0';

plugin.registerSemanticParser(1, 'ListParser', list_match);
plugin.registerSemanticParser(1, 'ListImagesParser', image_match);
