// Copyright © 2018 TermySequence LLC
// SPDX-License-Identifier: CC0-1.0
// This file is free to build upon, enhance, and reuse, without restriction.

//
// Tip of the Day provider
//

var strings = [
    // Tips with argument substitutions
    "Use keyboard shortcuts to quickly bring up <a doc='tools/index.html'>tools</a>. Your bindings are:<ul><li>RaiseKeymapTool: %1<li>RaiseTerminalsTool: %2<li>RaiseSuggestionsTool: %3<li>RaiseSearchTool: %4<li>RaiseFilesTool: %5<li>RaiseHistoryTool: %6<li>RaiseAnnotationsTool: %7<li>RaiseTasksTool: %8</ul>",
    "Your copy and paste bindings are:<ul><li>Copy: %1<li>Paste: %2</ul>",
    "Use presentation mode (View→Presentation Mode) to hide distractions and increase the terminal font size in a single step. Customize it in the <a act='EditGlobalSettings'>global settings</a> under <i>Presentation Mode</i>. Your TogglePresentationMode binding is %1.",
    "Your ToggleFullScreen binding is %1.",
    "Using View→Split View to create multiple terminal panes? Here are your bindings for switching focus between panes:<ul><li>PreviousPane: %1<li>NextPane: %2:<li>SwitchPane|0: %3<li>SwitchPane|1: %4<li>SwitchPane|2: %5<li>SwitchPane|3: %6</ul>",
    // 5
    "Your basic scroll bindings are:<ul><li>ScrollPageUp: %1<li>ScrollPageDown: %2<li>ScrollToTop: %3<li>ScrollToBottom: %4<li>ScrollLineUp: %5<li>ScrollLineDown: %6</ul>",
    "Your <a doc='shell-integration.html'>shell integration</a> scroll bindings are:<ul><li>ScrollPromptUp: %1<li>ScrollPromptDown: %2</ul>",
    "Here are your bindings to reorder terminals in the <a doc='tools/terminals.html'>Terminals tool</a>:<ul><li>ReorderTerminalBackward: %1<li>ReorderTerminalForward: %2<li>ReorderTerminalFirst: %3<li>ReorderTerminalLast: %4</ul>",
    "Here are your bindings to reorder servers in the <a doc='tools/terminals.html'>Terminals tool</a>:<ul><li>ReorderServerBackward: %1<li>ReorderServerForward: %2<li>ReorderServerFirst: %3<li>ReorderServerLast: %4</ul>",
    "Here are your quick font size adjustment bindings:<ul><li>IncreaseFont: %1<li>DecreaseFont: %2</ul>",
    // 10
    "Type three or more characters at the terminal prompt, then use WriteSuggestion (%1) to write the first suggestion in the <a doc='tools/suggestions.html'>Suggestions tool</a>. It's the \"I'm Feeling Lucky\" button for the terminal. Don't worry, no newline is written (there's a separate action for that, <a doc='actions.html#WriteSuggestionNewline'>WriteSuggestionNewline</a>.",
    "Here are your bindings to write the top suggestions in the <a doc='tools/suggestions.html'>Suggestions tool</a> to the terminal:<ul><li>WriteSuggestion|0: %1<li>WriteSuggestion|1: %2<li>WriteSuggestion|2: %3<li>WriteSuggestion|3: %4</ul>",
    "With the <a doc='tools/suggestions.html'>Suggestions tool</a> active, use SuggestNext and SuggestPrevious to change the selected suggestion, WriteSuggestion to write it to the terminal, and RemoveSuggestion to remove it from the database. Additionally, the generic actions ToolAction|0 and ToolAction|1 map to WriteSuggestion and RemoveSuggestion by default (though this can be changed in the <a act='EditGlobalSettings'>global settings</a>). Your suggestion-related bindings (note that \"Backtab\" is Shift-Tab) are:<ul><li>SuggestNext: %1<li>SuggestPrevious: %2<li>WriteSuggestion: %3<li>RemoveSuggestion: %4<li>ToolAction|0: %5<li>ToolAction|1: %6</ul>",
    "Use the following generic actions to change the selected item in the <a doc='tools/index.html'>active tool</a>:<ul><li>ToolPrevious: %1<li>ToolNext: %2<li>ToolFirst: %3<li>ToolLast %4</ul>Then, use the ToolAction action (%5) to run the primary (double-click) action on the selected item, as configured in the <a act='EditGlobalSettings'>global settings</a> for the tool.",
    "With <a doc='shell-integration.html'>shell integration</a> enabled, use the SelectOutput action (%1) to select the output of the last command.",
    // 15
    "The X11 select buffer, separate from the regular clipboard and traditionally pasted using middle-click, can be pasted using the PasteSelectBuffer action (%1).",
    "Here are your bindings for scrolling to <a doc='concepts-qt.html#term-annotation'>annotations</a>:<ul><li>ScrollNoteUp: %1<li>ScrollNoteDown: %2</ul>",
    "Here are your scrollback search bindings:<ul><li>Find: %1<li>SearchUp: %2<li>SearchDown: %3<li>SearchReset: %4</ul>",
    "With <a doc='shell-integration.html'>shell integration</a> enabled, scrolling to a previous prompt (%1) or clicking a command in the Marks widget, Minimap widget, or History tool will set the <a doc='concepts-qt.html#term-selected-prompt'>selected prompt</a> in the terminal, which is shown in a prominent color. Certain actions such as CopyJob or SelectOutput can target the job associated with the selected prompt.",
    "<b>qtermy</b> looks for <a doc='plugins/index.html'>plugins</a> in two locations: <tt>%1/share/qtermy/plugins</tt> and <tt>$HOME/.local/share/qtermy/plugins</tt>. User plugins override system plugins with the same name. <b>qtermy</b> can be run with the <i>--nosysplugins</i> command line argument to disable loading system plugins, or <i>--noplugins</i> to disable loading plugins entirely.",
    // 20
    "<b>qtermy</b> looks for SVG images in two locations: <tt>%1/share/qtermy/images</tt> and <tt>$HOME/.local/share/qtermy/images</tt>. All terminal and server thumbnail icons, semantic region icons, and emoji images are loaded from these folders. More images can be added by dropping them into the appropriate subdirectory and then restarting <b>qtermy</b>.",
    "Don't like the menu icons? Disable the icon theme in the <a act='EditGlobalSettings'>global settings</a>, create your own icon theme under <tt>%1/share/qtermy/icons</tt>, or replace individual icons in the default theme.",

    // Tips without argument substitutions
    "The mouse actions for each tool can be changed in the <a act='EditGlobalSettings'>global settings</a> and can be bound to keys using the ToolAction action, with the following relationship:<ul><li>Double-click action = ToolAction|0<li>Control-click action = ToolAction|1<li>Shift-click action = ToolAction|2<li>Middle-click action = ToolAction|3</ul>",
    "You can define custom <a doc='concepts-qt.html#term-emoji'>emoji</a> in the Unicode range f5000-f50ff. Simply place an SVG image into <tt>$HOME/.local/share/qtermy/images/emoji</tt> named after the code point, for example <tt>f5000.svg</tt>",
    "With <a doc='shell-integration.html'>shell integration</a> enabled, the following copy and select actions become available:<ul><li>CopyCommand/SelectCommand - copy or select the last command<li>CopyOutput/SelectOuput - copy or select output of the last command<li>CopyJob/SelectJob - copy or select the prompt, command, and output of the last command</ul>Arguments to these actions specify the <a doc='concepts-qt.html#term-job'>job</a> to act on. Refer to the <a doc='actions.html'>actions documentation</a> for more information.",
    "<b>qtermy</b> includes built-in dark and light <a href='http://ethanschoonover.com/solarized'>Solarized</a> color themes, including Solarized dircolors. Try them out!",
    "The <a act='RandomTerminalTheme'>RandomTerminalTheme</a> action normally chooses from all color themes, but it can be told to choose only <a doc='settings/theme.html'>themes</a> in the same group as the current theme. Bind a key to <a act='RandomTerminalTheme|1'>RandomTerminalTheme|1</a> to use it in this mode.",
    // 5
    "All <a doc='settings/theme.html'>themes</a> with the \"low priority\" setting enabled will be placed at the bottom of the list in the <a doc='dialogs/theme-editor.html'>theme editor</a>. This is intended for themes that only exist to be used with the <a act='RandomTerminalTheme'>RandomTerminalTheme</a> action.",
    "The ga, gm, gi, and gu <a doc='dialogs/dircolors-editor.html'>dircolors categories</a> are TermySequence extensions that allow setting the color of the git markers in the <a doc='tools/files.html'>Files tool</a> for added, modified, ignored, and unmerged files, respectively.",
    "\"Placeholder\" <a doc='dialogs/dircolors-editor.html'>dircolors entries</a> are a TermySequence extension that allows a named placeholder to be substituted into file glob entries. This allows the same entry to be assigned to many different file extensions and then changed in one place.",
    "A <a doc='dialogs/dircolors-editor.html'>dircolors string</a> that begins with \"+\" will inherit the default compiled-in dircolors. Remove the + to use only the specified dircolors. Use the dircolors string \"rs=0\" to assign no dircolors at all.",
    "<a href='https://www.iterm2.com/documentation-shell-integration.html'>iTerm2 shell integration</a> is required to use many TermySequence features. Check the Terminal Properties tab in the <a act='ViewTerminalInfo'>Terminal Information window</a> to see if shell integration is enabled in the terminal.",
    // 10
    "Lines can be drawn in the terminal at any column position using the <a act='AdjustTerminalLayout'>Adjust Layout dialog</a> or the Column Fills <a act='EditProfile'>profile setting</a> under <i>Appearance</i>. Combine this with <a act='EditSwitchRules'>profile autoswitch rules</a> to draw a line at the 80 character position when <b>vim</b> is running in the terminal.",
    "Timestamps for each row in the terminal can be viewed using the <a doc='widgets.html#timing'>Timing widget</a>. Use the context menu to fix the timing origin at a specific row, or float it to track the most recent command.",
    "Create key bindings for string literals that you type often, such as email addresses, paths, URL's, hostnames, or usernames. Use two-keystroke combination bindings to make many strings available under a single primary keystroke.",
    "Online documentation for any setting can be obtained by clicking the question mark link next to the setting in the <a doc='dialogs/settings-editor.html'>settings editor</a>. Hover over the link to see the URL.",
    "Many <a doc='dialogs/index.html'>dialog boxes</a> have a Help button to bring up online documentation. Hover over the button to see the URL.",
    // 15
    "Changes to some <a act='EditGlobalSettings'>global settings</a> do not take effect until <b>qtermy</b> is restarted.",
    "The <a act='EditIconRules'>icon autoswitch rules</a> are also used to set icons for the commands in the <a doc='tools/history.html'>History tool</a>. Only rules that match against the <i>proc.comm</i> variable are used for this purpose.",
    "Left-click the terminal size in the status bar to bring up the <a act='ViewTerminalInfo'>Terminal Information window</a>.",
    "Left-click the profile name in the status bar to <a act='EditProfile'>edit</a> the current <a doc='settings/profile.html'>profile</a>. Right-click to change profiles.",
    "Hover over the abbreviated flag names in the status bar to get a description of each flag.",
    // 20
    "Terminals are created using the size configured in the <a act='EditProfile'>profile settings</a> under <i>Emulator</i>. For best results, set this size to the usual size of your terminal window.",
    "TermySequence treats terminals as global objects, with each window pane being a viewport onto a terminal. Multiple viewports can be opened on the same terminal and scrolled independently.",
    "The terminal thumbnail image always shows the terminal screen at its native size, regardless of the size and position of the current viewport.",
    "The number next to the server icon in the <a doc='tools/terminals.html'>Terminals tool</a> is the total number of terminals on that server. If any terminals are hidden, the number of hidden terminals is shown as a second number in parentheses.",
    "Thumbnail captions, thumbnail tooltips, window titles, and terminal badges can be customized to include any terminal or server attribute. Use the <a act='ViewTerminalInfo'>Terminal Information window</a> to view the available attributes.",
    // 25
    "The <a doc='tools/history.html'>History tool</a> and <a doc='widgets.html#marks'>Marks widget</a> use the following convention to display an exit status or signal number:<ul><li>0-9 are shown as 0-9<li>10-35 are shown as a-z<li>35-126 are shown as +<li>127 is shown as -</ul>",
    "Place an SVG image at <tt>$HOME/.config/qtermy/avatar.svg</tt> and other users will see it displayed on your terminals.",
    "Create custom dircolors for the <a doc='tools/files.html'>Files tool</a> using the <a doc='dialogs/dircolors-editor.html'>Dircolors editor</a>. Then, configure the <a act='EditProfile'>profile setting</a> under <i>Files</i> to set the <tt>LS_COLORS</tt> environment variable in newly created terminals.",
    "If you frequently run a command that prints a small amount of human-readable text, try creating a <a doc='settings/launcher.html'>launcher</a> to run it with output to a dialog box. Then, create a key binding for LaunchCommand|name to run the command on demand without cluttering the terminal (<b>fortune</b> is a great command to run this way).",
    "Install <b>termy-server</b> in your containers and use TermySequence <a act='ManageConnections'>connections</a> to get arbitrary numbers of terminals, file transfers, command <a doc='settings/launcher.html'>launching</a> and more over a single container connection.",
    // 30
    "TermySequence supports iTerm2 style inline image display, with some restrictions on image size. Check out the <a doc='man/download.html'>termy-imgcat</a> and <a doc='man/download.html'>termy-imgls</a> scripts that come with <b>termy-server</b>.",
    "Use <b>qtermy</b>'s FUSE3 support to directly edit files on remote servers and in containers using local desktop applications. Or, use a local <b>sudo</b> or <b>su</b> <a doc='settings/connection.html'>connection</a> to directly edit root-owned files on the local machine.",
    "The default profile and list of startup terminals can be set on a per-server basis in the <a doc='settings/server.html'>server settings</a>.",
    "A <a doc='dialogs/connect-batch.html'>batch connection</a> can open multiple connections, including connections to remote servers across multiple hops, all in a single step.",
    "Bind a key to ToggleSelectionMode to start a text selection using the keyboard. Then, edit the selection using further Selection Mode key bindings, and configure the <a act='EditGlobalSettings'>global setting</a> under <i>Selection Mode</i> to exit Selection Mode automatically when performing a copy. All without touching the mouse.",
    // 35
    "The <a doc='actions.html#SaveScreen'>SaveScreen</a> action can save both text and PNG images. It can also be given an output file path as an argument. Take terminal screenshots using a single keystroke, with no tedious cropping required afterwards.",
    "The <a doc='actions.html#CopyScreen'>CopyScreen</a> action can copy both text and PNG images to the clipboard.",
    "Have a lot of terminals and servers open? Use the <a act='ManageTerminals'>Manage Terminals window</a> (File→Manage Terminals) to get a tree view suitable for bulk management of terminals.",
    "Each <a doc='settings/profile.html'>profile</a> has its own independent color palette. Editing a <a doc='settings/theme.html'>theme</a> will not change the color palette in any profile. A profile palette that does not match any theme will be labeled \"Custom Color Theme\"",
    "The most recently favorited <a act='ManageConnections'>connections</a> of each type will be placed directly in the Connect menu for the quickest access.",
    // 40
    "The \"prefer transient server\" <a act='EditGlobalSettings'>global setting</a> under <i>Server</i> controls which <a doc='concepts-qt.html#term-local-server'>local server</a> the NewLocalTerminal action will use when both local servers are connected.",
    "The <a act='NewWindow'>New Window</a> entry in the File menu will not create a new terminal along with the new window. The New Window entry in the Server menu, however, <i>will</i> create a new terminal along with the new window.",
    "Use <a doc='concepts-qt.html#term-input-multiplexing'>input multiplexing</a> (Edit→Input Multiplexing) to send input to many terminals at once. While active, all input to the \"input leader\" will be copied to each \"input follower\".",
    "Use Command Mode to implement a vim-style dual mode keymap that doesn't require modifier keys. In the <a act='EditKeymap'>keymap editor</a>, set the \"Command\" condition to true in any binding that will only be used in Command Mode.",
    "Create a key binding to OpenConnection|name to launch a saved <a doc='settings/connection.html'>connection</a> with a single keystroke. You won't miss typing in SSH commands from scratch.",
    // 45
    "Both task status dialogs and the <a doc='tools/tasks.html'>Tasks tool</a> can be automatically lowered after a task finishes, with a short delay time, using the <a act='EditGlobalSettings'>global settings</a> under <i>Tasks Tool</i>.",
    "Use the <a act='ManagePortForwarding'>Port Forwarding Manager</a> (Server→Port Forwarding) to create arbitrary port forwarding between the client and any connected server on demand. Saved port forwarding rules in a server's <a doc='settings/server.html'>settings</a> can be automatically launched whenever a connection to the server is made.",
    "Connections can be opened from <b>qtermy</b> itself or from any connected server using the <i>LaunchFrom</i> option in the <a doc='settings/connection.html'>connection settings</a>. Connections can also be opened directly from a terminal using <a doc='man/connect.html'>termy-connect(1)</a>.",
    "Initiate a <a doc='settings/connection.html'>connection</a> from the <a doc='concepts-qt.html#term-persistent-user-server'>persistent user server</a> rather than from <b>qtermy</b> itself and it will stay open even if <b>qtermy</b> is closed and restarted.",
    "Use the <a act='EditProfile'>profile settings</a> under <i>Collaboration</i> to control whether you see other users' fonts and colors in their terminals. Like what you see? Use Settings→Extract Profile to extract a <a doc='settings/profile.html'>profile</a> from the terminal.",
    // 50
    "To experiment with collaboration using TermySequence, set up a shared user account on a server with <b>termy-server</b> installed, then have multiple people connect to it using <b>qtermy</b>.",
    "File uploads and downloads to a <a doc='concepts-qt.html#term-local-server'>local server</a> can be enabled in the <a act='EditGlobalSettings'>global settings</a> under <i>Server</i>, but FUSE mounts of local files and directories are not permitted.",
    "<b>qtermy</b> downloads scrollback contents to ensure no lines are missed. The download progress is indicated by a triangular cursor in the minimap widget, which can be disabled in the <a act='EditProfile'>profile settings</a> under <i>Appearance</i>. The download speed can also be configured in the <a act='EditGlobalSettings'>global settings</a> under <i>Server</i>.",
    "All of <b>qtermy</b>'s <a doc='settings/index.html'>settings</a> are saved as INI or text files under <tt>$HOME/.config/qtermy</tt> and can be exchanged with other users. Settings files can be edited using text editors, but <b>qtermy</b> does not monitor for external changes to settings files. Restart <b>qtermy</b> after making changes.",
    "The command history used by the <a doc='tools/suggestions.html'>Suggestions tool</a> is saved at <tt>$HOME/.cache/qtermy/history.sqlite3</tt>. It can be opened and examined using any sqlite3 editor.",
    // 55
    "Use the <a act='RaiseKeymapTool'>Keymap tool</a> to see the bindings in the current <a doc='settings/keymap.html'>keymap</a> at a glance.",
    "An asterisk next to a command in the <a doc='tools/suggestions.html'>Suggestions tool</a> indicates that the exact string typed at the prompt was last completed to that command. This command will be made the top suggestion regardless of its frecency score.",
    "The <a doc='tools/suggestions.html'>Suggestions tool</a> is capable of matching commands based on the first alphanumeric character of each word. The string \"ggh\" typed at the prompt will match \"git grep hello\".",
    "The <a doc='tools/suggestions.html'>Suggestions tool</a> is capable of matching commands anywhere in the string, not just at the front. The string \"hello\" typed at the prompt will match \"git grep hello\".",
    "High-numbered mouse buttons can be assigned to bindings in the <a act='EditKeymap'>keymap editor</a>, just like keys.",
    // 60
    "TermySequence has two kinds of scroll lock. Hard scroll lock is the result of sending a STOP character (normally DC3, Ctrl+S) to the terminal driver and is not implemented by <b>termy-server</b>. It can be undone using the START character (normally DC1, Ctrl+Q). Soft scroll lock is the result of the ToggleSoftScrollLock action (bound to Scroll Lock by default), which <i>is</i> implemented by <b>termy-server</b> (it simply stops reading output from the terminal). The server attempts to detect and report the STOP sequence so that <b>qtermy</b> can warn when either mode is active.",
    "Help→Highlight Cursor will display a brief animation at the cursor location. Frequently lose sight of the cursor? Bind a key to the <a act='HighlightCursor'>HighlightCursor</a> action.",
    "Help→Highlight Inline Contents will display a brief animation over any links or <a doc='concepts-qt.html#term-semantic-region'>semantic regions</a> in the terminal viewport. Frequently use this feature? Bind a key to the <a act='HighlightSemanticRegions'>HighlightSemanticRegions</a> action.",
    "<b>qtermy</b> embeds the <a href='https://developers.google.com/v8/'>Chrome V8 Javascript engine</a> by Google and provides a plugin API which can be used to implement <a doc='plugins/action.html'>custom actions</a> as well as <a doc='plugins/parser.html'>semantic parsers</a> which create semantic regions within command output in the terminal. Consult the <a doc='plugins/index.html'>online documentation</a> and the sample plugins included with <b>qtermy</b> for more information.",
    "<b>termy-server</b> launches separate \"monitor\" programs to report interesting server attributes. These can be overridden or customized to report your own attributes, which can then be displayed in thumbnail captions, thumbnail tooltips, window titles, or terminal badges. It's also possible to send commands to the monitor program using the <a doc='actions.html#SendMonitorInput'>SendMonitorInput</a> action, which can be used to trigger custom reporting. Refer to <a act='ManpageTerminal|termy-monitor'>termy-monitor(1)</a> for more information.",
    // 65
    "<b>qtermy</b> provides hundreds of actions which are used to implement all of its menus and key bindings. Each action invocation has a name and one or more arguments separated by vertical bar (|) characters. Consult the <a doc='actions.html'>actions documentation</a> for more information.",
    "<b>qtermy</b> ships with <a doc='plugins/parser.html'>semantic parser plugins</a> that can parse the output of several container list commands:<ul><li>docker images|image ls|ps<li>kubectl get pods|get deployments<li>machinectl list|list-images</ul>Try them out with your own container installation. Note that the \"Connect\" option in the semantic context menu requires <b>termy-server</b> to be installed in the container (which is highly recommended).",
    "<b>termy-server</b> is fully enabled for use with the systemd user service manager via socket activation. To enable socket activation for a user, run <a doc='man/setup.html'>termy-systemd-setup</a>.",
    "Any task which produces a local file provides a number of file-related options in its context menu, which can be accessed from the <a doc='tools/tasks.html'>Tasks tool</a> or from the task status dialog:<ul><li>Open the file using any launcher<li>Open the file's enclosing folder in the desktop file browser<li>Open a terminal to the enclosing directory<li>Copy the file's full path or enclosing directory to the clipboard or write it to the active terminal</ul>",
    "Remember that the <a doc='concepts-qt.html#term-persistent-user-server'>persistent user server</a> is independent of and outside the current desktop session, so <tt>DISPLAY</tt> and other session-specific environment variables will generally not be set within its terminals. Applications that require these variables should be run from the <a doc='concepts-qt.html#term-transient-local-server'>transient local server</a>, which is a child process of <b>qtermy</b> and thus <i>does</i> belong to the current desktop session.",
    // 70
    "<b>qtermy</b> has a logging system which writes log messages to the <a act='EventLog'>Event Log window</a> (Help→Event Log) and to Qt's default logging handler which prints on standard output. The file <tt>$HOME/.config/QtProject/qtlogging.ini</tt> can be used to filter log messages sent to the default handler (refer to the <a href='http://doc.qt.io/qt-5/qloggingcategory.html'>Qt documentation</a> for more information). Or, disable the default handler entirely in the <a act='EditGlobalSettings'>global settings</a> under <i>General</i>.",
    "<b>qtermy</b> handles the alternate screen buffer, a separate terminal screen used by most fullscreen terminal applications such as <b>vim</b>, by placing it below the rest of the terminal scrollback separated by a horizontal line. While the alternate screen buffer is active, it is still possible to scroll up and view the terminal scrollback. However note that if alternate scroll mode is enabled in the terminal, the mouse wheel will send input to the terminal instead of scrolling the terminal scrollback as it normally does. Alternate scroll mode can be disabled in the <a act='EditProfile'>profile settings</a> under <i>Input</i>.",
    "With <a doc='shell-integration.html'>shell integration</a> enabled, two useful <a act='EditProfile'>profile settings</a> become available under <i>Emulator</i>. The \"force newline\" setting writes a newline before printing a prompt if the prior command  did not print one. The \"clear screen by scrolling\" setting writes newlines to the terminal instead of clearing the screen when a prompt is active, preserving the contents of the scrollback when for example Ctrl+L is pressed at a prompt.",
    "If a terminal is closing too quickly after being created, try adjusting the \"minimum runtime required before auto-close\" <a act='EditProfile'>profile setting</a> under <i>Emulator</i>.",
    "When viewing a terminal belonging to another user, use Terminal→Scroll with Owner to track the other user's scrollback position. If the other user scrolls up to look at something, you will too.",
    // 75
    "The \"pack terminal thumbnails closely together\" <a act='EditGlobalSettings'>global setting</a> under <i>Appearance</i> reduces the amount of space allocated for terminal thumbnail captions. It has no effect if the terminal thumbnail caption is empty, which is the default.",
    "Up to 8 profiles can be pushed in a terminal. Popping a profile will switch back to the previous profile on the stack. This is useful when using <a act='EditSwitchRules'>profile autoswitch rules</a> to change the current profile only for the duration of a specific command.",
    "The Tools menu displays menu entries in the context of the <a doc='tools/index.html'>active tool</a>. A tool is made active by clicking anywhere in it, by running actions specific to the tool, or in some cases in response to external events. For example, the Tasks tool can raise itself upon a task finishing. Tool auto-raising is configured in the <a act='EditGlobalSettings'>global settings</a>.",
    "The <a doc='tools/files.html'>Files tool</a> is only capable of showing the terminal's current directory. When the terminal's foreground process changes the current directory, the Files tool will change with it. However, if <a doc='shell-integration.html'>shell integration</a> is enabled, double-clicking a folder name in the Files tool will write a <b>cd</b> command to the terminal by default, making it possible to navigate around using the Files tool.",
    "The <a doc='tools/files.html'>Files tool</a> has two display formats: short, similar to <b>ls</b>, and long, similar to <b>ls -l</b>. Configure the default display format and other file display options in the <a act='EditProfile'>profile settings</a> under <i>Files</i>.",
    // 80
    "Semantic information about a command is lost when the command scrolls off the top of the scrollback buffer, even if some command output is still visible. Adjust your scrollback size to the length of the longest command output that you anticipate needing to review. For improved performance and memory use, use a smaller scrollback buffer.",
    "With <a doc='shell-integration.html'>shell integration</a> enabled, a certain number of recent prompts are shown in the <a doc='widgets.html#minimap'>Minimap widget</a>. This can be adjusted in the <a act='EditProfile'>profile settings</a> under <i>Appearance</i>. For more history at a glance, show more prompts. For improved performance, show fewer prompts.",
    "Window position and geometry information is saved in the <a doc='settings/state.html'>State settings</a> at <tt>$HOME/.config/qtermy/qtermy.state</tt>. This file can be edited or removed to make <b>qtermy</b> \"forget\" the saved state. Saving and restoring of state information can be configured in the <a act='EditGlobalSettings'>global settings</a> under <i>General</i>.",
    "Create a key binding for the string <i>git commit -m \"\"\\E[D</i>. When used at a typical shell prompt, this will first enter the command, then move the cursor between the quotes where the commit message should go.",
    "To quickly invoke actions without key bindings, bind a key to the <a act='Prompt'>Prompt</a> action, which opens a dialog from which any action (including a <a doc='plugins/action.html'>custom action</a>) can be invoked.",
];

var total = strings.length;
var cur = 0;
var order = null;

function generateRandomOrder() {
    var indices = Array.from(Array(total).keys());
    order = new Array(total);

    for (var i = 0; i < total; ++i) {
        var k = Math.floor(Math.random() * (total - i));
        order[i] = indices.splice(k, 1)[0];
    }
}

function getActiveKeymap(manager) {
    var term = manager.getActiveTerminal();
    var profile = term ? term.getProfile() : manager.getDefaultProfile();
    return profile.getKeymap();
}

function getKey(keymap, slot) {
    var obj = keymap.lookupShortcut(slot);
    return obj ? plugin.htmlEscape(obj.expression) : 'unbound';
}

function getNextTip(manager) {
    if (order == null) {
        generateRandomOrder();
    } else {
        cur = (cur + 1) % total;
    }

    var idx = order[cur];
    var str = strings[idx];
    var km = getActiveKeymap(manager);

    // Get argument substitutions
    switch (idx) {
    case 0:
        return [ str, getKey(km, 'RaiseKeymapTool'), getKey(km, 'RaiseTerminalsTool'), getKey(km, 'RaiseSuggestionsTool'), getKey(km, 'RaiseSearchTool'), getKey(km, 'RaiseFilesTool'), getKey(km, 'RaiseHistoryTool'), getKey(km, 'RaiseAnnotationsTool'), getKey(km, 'RaiseTasksTool') ];
    case 1:
        return [ str, getKey(km, 'Copy'), getKey(km, 'Paste') ];
    case 2:
        return [ str, getKey(km, 'TogglePresentationMode') ];
    case 3:
        return [ str, getKey(km, 'ToggleFullScreen') ];
    case 4:
        return [ str, getKey(km, 'PreviousPane'), getKey(km, 'NextPane'), getKey(km, 'SwitchPane|0'), getKey(km, 'SwitchPane|1'), getKey(km, 'SwitchPane|2'), getKey(km, 'SwitchPane|3') ];
    case 5:
        return [ str, getKey(km, 'ScrollPageUp'), getKey(km, 'ScrollPageDown'), getKey(km, 'ScrollToTop'), getKey(km, 'ScrollToBottom'), getKey(km, 'ScrollLineUp'), getKey(km, 'ScrollLineDown') ];
    case 6:
        return [ str, getKey(km, 'ScrollPromptUp'), getKey(km, 'ScrollPromptDown') ];
    case 7:
        return [ str, getKey(km, 'ReorderTerminalBackward'), getKey(km, 'ReorderTerminalForward'), getKey(km, 'ReorderTerminalFirst'), getKey(km, 'ReorderTerminalLast') ];
    case 8:
        return [ str, getKey(km, 'ReorderServerBackward'), getKey(km, 'ReorderServerForward'), getKey(km, 'ReorderServerFirst'), getKey(km, 'ReorderServerLast') ];
    case 9:
        return [ str, getKey(km, 'IncreaseFont'), getKey(km, 'DecreaseFont') ];
    case 10:
        return [ str, getKey(km, 'WriteSuggestion') ];
    case 11:
        return [ str, getKey(km, 'WriteSuggestion|0'), getKey(km, 'WriteSuggestion|1'), getKey(km, 'WriteSuggestion|2'), getKey(km, 'WriteSuggestion|3') ];
    case 12:
        return [ str, getKey(km, 'SuggestNext'), getKey(km, 'SuggestPrevious'), getKey(km, 'WriteSuggestion'), getKey(km, 'RemoveSuggestion'), getKey(km, 'ToolAction|0'), getKey(km, 'ToolAction|1') ];
    case 13:
        return [ str, getKey(km, 'ToolPrevious'), getKey(km, 'ToolNext'), getKey(km, 'ToolFirst'), getKey(km, 'ToolLast'), getKey(km, 'ToolAction|0') ];
    case 14:
        return [ str, getKey(km, 'SelectOutput') ];
    case 15:
        return [ str, getKey(km, 'PasteSelectBuffer') ];
    case 16:
        return [ str, getKey(km, 'ScrollNoteUp'), getKey(km, 'ScrollNoteDown') ];
    case 17:
        return [ str, getKey(km, 'Find'), getKey(km, 'SearchUp'), getKey(km, 'SearchDown'), getKey(km, 'SearchReset') ];
    case 18:
        return [ str, getKey(km, 'ScrollPromptUp') ];
    case 19:
    case 20:
    case 21:
        return [ str, plugin.htmlEscape(plugin.installPrefix) ];
    default:
        return str;
    }
}

if (plugin.majorVersion != 1) {
    throw new Error("unsupported API version");
}

plugin.pluginName = 'TipOfTheDayPlugin';
plugin.pluginDescription = 'Provides tip of the day messages';
plugin.pluginVersion = '1.0';

plugin.registerTipProvider(1, getNextTip);
