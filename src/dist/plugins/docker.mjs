// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

// Command alias
// To enable this parser for an alias, enter the alias here:
// var images_alias = 'di';
var images_alias = '';
var ps_alias = '';

function ContainerParams(parser, name) {
    this.icon = 'container';
    this.tooltip = 'Double-click to open connection';
    this.action1 = [ 'NewConnection', '10', name, parser.server ].join('|');
    this.drag = { 'text/plain':name };

    var cmdspec = [ 'docker', 'docker', 'exec', '-ti', name, '/bin/bash' ];
    var run = [ 'CommandTerminal', '', parser.server, cmdspec.join('\x1f') ];

    this.menu = [
        [ 1, this.action1, '&Connect', 'connection-launch',
          'Open a connection to this container' ],
        [ 1, run.join('|'), '&Open Shell', 'new-terminal',
          'Run bash in this container in a new terminal' ],
        [ 0 ]
    ];

    cmdspec = [ 'docker', 'docker', 'inspect', name ];
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Inspect', 'inspect-item',
                     'Get container status'], [ 0 ]);

    cmdspec[2] = 'stop';
    run = [ 'RunCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Stop', 'shutdown',
                     'Stop the container']);
    this.menu.push([ 2, 'Really stop container ' + name + '?' ]);

    cmdspec[2] = 'restart';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Restart', 'reboot',
                     'Restart the container']);

    cmdspec[2] = 'pause';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Pause', 'pause',
                     'Pause the container']);

    cmdspec[2] = 'unpause';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Unpause', 'resume',
                     'Unpause the container'], [ 0 ]);

    cmdspec = [ 'docker', 'docker', 'info' ];
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Docker Info', 'help-about',
                     'Show information about the docker installation']);

    cmdspec[2] = 'version';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), 'Docker &Version', '',
                     'Show information about the docker version']);
}

function ImageParams(parser, name) {
    var cmdspec = [ 'docker', 'docker', 'image', 'inspect', name ];
    var run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];

    this.icon = 'container';
    this.tooltip = 'Double-click to show image information';
    this.action1 = run.join('|');
    this.drag = { 'text/plain':name };

    this.menu = [
        [ 1, this.action1, '&Inspect Image', 'inspect-item',
          'Show image information' ],
        [ 0 ]
    ];

    cmdspec = [ 'docker', 'docker', 'info' ];
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Docker Info', 'help-about',
                     'Show information about the docker installation']);

    cmdspec[2] = 'version';
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), 'Docker &Version', '',
                     'Show information about the docker version']);
}

// Split line into words
var splitre = /\S+/g;

function ps_process(context, text) {
    var result = text.match(splitre);
    if (result) {
        if (result[0] == 'CONTAINER') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new ContainerParams(this, result[0]));
        }
    }
    return true;
}

function image_process(context, text) {
    var result = text.match(splitre);
    if (result) {
        if (result[0] == 'REPOSITORY') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new ImageParams(this, result[2]));
        }
    }
    return true;
}

function Parser(process_func) {
    this.start = function(context, exitcode) { return exitcode == 0 };
    this.process = process_func;
    this.state = false;
}

function ps_match(context, cmdline) {
    if (cmdline != 'docker ps')
        if (!ps_alias || cmdline != ps_alias)
            return null;

    var parser = new Parser(ps_process);
    parser.server = context.getServerId();
    return parser;
}

function image_match(context, cmdline) {
    if (cmdline != 'docker images' && cmdline != 'docker image ls')
        if (!images_alias || cmdline != images_alias)
            return null;

    var parser = new Parser(image_process);
    parser.server = context.getServerId();
    return parser;
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'DockerPlugin';
plugin.pluginDescription = 'Creates semantic regions in docker output';
plugin.pluginVersion = '1.0';

plugin.registerSemanticParser(1, 'PsParser', ps_match);
plugin.registerSemanticParser(1, 'ImagesParser', image_match);
