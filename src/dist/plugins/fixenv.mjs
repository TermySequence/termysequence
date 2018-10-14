// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

//
// A custom action that writes "eval `termyctl fix-env`" to all terminals
// with active shell prompts on the same server as the active terminal
//
// To use the custom action defined in this plugin, bind the name
// "CustomFixEnv" to a key in the keymap editor.
//

function actionHandler(manager, sameGroup) {
    let server = manager.getActiveServer();
    if (!server)
        return;

    let action = 'WriteTextNewline|eval `termyctl fix-env`|';

    // Only write to terminals that:
    // - we have ownership of
    // - have a shell integration prompt active
    for (let term of server.listTerminals()) {
        if (term.ours && term.getAttribute('command') !== undefined) {
            manager.invoke(action + term.id);
        }
    }
}

if (plugin.majorVersion != 1 || plugin.minorVersion < 3) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'FixEnvPlugin';
plugin.pluginDescription = 'Write fix-env command to terminals';
plugin.pluginVersion = '1.0';

plugin.registerCustomAction(1, 'FixEnv', actionHandler)
