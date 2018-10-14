// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

//
// A custom action implementing theme rotation in the active terminal
//
// To use the custom action defined in this plugin, bind the name
// "CustomRotateTheme" to a key in the keymap editor. Or use
// "CustomRotateTheme|1" to only rotate through themes in the same
// group as the current theme.
//

function actionHandler(manager, sameGroup) {
    let term = manager.getActiveTerminal();
    if (!term)
        return;

    let themeMap = manager.listThemes();
    let themeNames = Array.from(themeMap.keys()).sort();
    let themes = themeNames.map(name => themeMap.get(name));
    let theme = term.getTheme();

    if (sameGroup && theme) {
        var group = theme.getSetting('Theme/Group');
        themes = themes.filter(t => t.getSetting('Theme/Group') == group);
    }

    themes = themes.filter(t => !t.getSetting('Theme/LowPriority'))
    if (themes.length) {
        let idx = (themes.indexOf(theme) + 1) % themes.length;
        term.palette = themes[idx].getPalette();
    }
}

if (plugin.majorVersion != 1 || plugin.minorVersion < 2) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'RotateThemePlugin';
plugin.pluginDescription = 'Rotate through themes in order';
plugin.pluginVersion = '1.1';

plugin.registerCustomAction(1, 'RotateTheme', actionHandler)
