// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

//
// A custom action implementing rectangle copy
//
// To use the custom action defined in this plugin, bind the name
// "CustomRectangleCopy" to a key in the keymap editor. Select some
// text, then use the action to copy the selected inner rectangle
//

function actionHandler(manager) {
    var viewport = manager.getActiveViewport();
    if (!viewport)
        return;
    var selection = viewport.getSelection();
    if (!selection)
        return;

    var start = selection.getStart();
    var end = selection.getEnd();
    var left = start.offset;
    var right = end.offset;
    var lines = [];

    if (left < right) {
        viewport.createFlash(start, start.clone().moveToOffset(right));

        right -= left;
        while (true) {
            lines.push(start.text.substr(left, right));
            if (start.row == end.row)
                break;
            start.moveByRows(1);
        }

        viewport.createFlash(start.moveToOffset(left), end);
    }

    manager.copy(lines.join('\n'));
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'RectangleCopyPlugin';
plugin.pluginDescription = 'Provides rectangle copy of selected text';
plugin.pluginVersion = '1.0';

plugin.registerCustomAction(1, 'RectangleCopy', actionHandler);
