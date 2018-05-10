// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

// Command alias
// To enable this parser for an alias, enter the alias here:
// var pods_alias = 'kp';
var pods_alias = '';
var deployments_alias = '';

function PodParams(parser, name) {
    this.icon = 'container';
    this.tooltip = 'Double-click to open connection';
    this.action1 = [ 'NewConnection', '11', name, parser.server ].join('|');
    this.drag = { 'text/plain':name };

    var cmdspec = [ 'kubectl', 'kubectl', 'exec', name, '-ti', '--', '/bin/bash' ];
    var run = [ 'CommandTerminal', '', parser.server, cmdspec.join('\x1f') ];

    this.menu = [
        [ 1, this.action1, '&Connect', 'connection-launch',
          'Open a connection to the default container' ],
        [ 1, run.join('|'), '&Open Shell', 'new-terminal',
          'Run bash in the default container in a new terminal' ],
        [ 0 ],
    ];

    cmdspec = [ 'kubectl', 'kubectl', 'describe', 'pod', name ];
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Describe', 'inspect-item',
                     'Show pod information'], [ 0 ]);

    cmdspec[2] = 'delete';
    run = [ 'RunCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Stop', 'shutdown',
                     'Delete the pod' ],
                   [ 2, 'Really delete pod ' + name + '?' ],
                   [ 0 ]);

    cmdspec = [ 'kubectl', 'kubectl', 'config', 'view'];
    run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Kubectl Config View', 'help-about',
                     'Show kubectl configuration']);

    cmdspec = [ 'kubectl', 'kubectl', 'version' ];
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), 'Kubectl &Version', '',
                     'Show kubectl version']);
}

function DeploymentParams(parser, name) {
    var cmdspec = [ 'kubectl', 'kubectl', 'describe', 'deployment', name ];
    var run = [ 'PopupCommand', parser.server, cmdspec.join('\x1f') ];

    this.icon = 'container';
    this.tooltip = 'Double-click to show deployment information';
    this.action1 = run.join('|');
    this.drag = { 'text/plain':name };

    this.menu = [
        [ 1, this.action1, '&Describe', 'help-about',
          'Show deployment information' ], [ 0 ]
    ];

    cmdspec[2] = 'delete';
    run = [ 'RunCommand', parser.server, cmdspec.join('\x1f') ];
    this.menu.push([ 1, run.join('|'), '&Stop', 'shutdown',
                     'Delete the deployment'],
                   [ 2, 'Really delete deployment ' + name + '?' ],
                   [ 0 ]);

    cmdspec = [ 'kubectl', 'kubectl', 'config', 'view'];
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), '&Kubectl Config View', 'help-about',
                     'Show kubectl configuration']);

    cmdspec = [ 'kubectl', 'kubectl', 'version' ];
    run[2] = cmdspec.join('\x1f');
    this.menu.push([ 1, run.join('|'), 'Kubectl &Version', '',
                     'Show kubectl version']);
}

// Split line into words
var splitre = /\S+/g;
// Match 'get pod' command
var podre = /^kubectl get pods?/;
// Match 'get deployment' command
var deploymentre = /^kubectl get deployments?/;

function deployments_process(context, text) {
    var result = text.match(splitre);
    if (result) {
        if (result[0] == 'NAME') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new DeploymentParams(this, result[0]));
        }
    }
    return true;
}

function pods_process(context, text) {
    var result = text.match(splitre);
    if (result) {
        if (result[0] == 'NAME') {
            this.state = true; // header seen
        } else if (this.state) {
            context.createRegion(0, text.length,
                                 new PodParams(this, result[0]));
        }
    }
    return true;
}

function Parser(process_func) {
    this.start = function(context, exitcode) { return exitcode == 0 };
    this.process = process_func;
    this.state = false;
}

function deployments_match(context, cmdline) {
    if (!deploymentre.test(cmdline))
        if (!deployments_alias || cmdline != deployments_alias)
            return null;

    var parser = new Parser(deployments_process);
    parser.server = context.getServerId();
    return parser;
}

function pods_match(context, cmdline) {
    if (!podre.test(cmdline))
        if (!pods_alias || cmdline != pods_alias)
            return null;

    var parser = new Parser(pods_process);
    parser.server = context.getServerId();
    return parser;
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'KubectlPlugin';
plugin.pluginDescription = 'Creates semantic regions in kubectl output';
plugin.pluginVersion = '1.0';

plugin.registerSemanticParser(1, 'GetPodsParser', pods_match);
plugin.registerSemanticParser(1, 'GetDeploymentsParser', deployments_match);
