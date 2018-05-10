#!/bin/bash
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: CC0-1.0

# This file is free to build upon, enhance, and reuse, without restriction.

function join_z { perl -e 'print join("\0", @ARGV)' "$@"; }
function join_c { perl -e 'print join(":", @ARGV)' "$@"; }

url='https://termysequence.io'

icon=$(echo -n error | base64)
drag=$(join_z 'text/uri-list' $url 'text/plain' $url | base64)
action=$(echo -n "OpenDesktopUrl|$url" | base64)
tooltip=$(echo -n 'Try double-clicking, right-clicking, or dragging me!' | base64)
menu=$(join_z \
       1 'WriteFilePath|Hello' 'Say Hello' 'paste' 'Write a Hello to the current terminal' \
       1 'RunCommand||sleep 30' 'Go to Sleep' 'execute' 'Run a sleep command on the current server' \
       0 '' '' '' '' \
       1 'CloneTerminal' 'Open a Terminal' 'new-terminal' 'Clone the current terminal' \
       1 'RandomTerminalTheme' 'Random Color Theme' 'random-theme' 'Use a random color theme in the current terminal' \
       1 'IncreaseFont' 'Increase Font Size' 'increase-font' 'Increase the font size in the current terminal' \
       0 '' '' '' '' \
       1 'EventLog' 'Show Event Log' 'event-log' 'Show the application event log!' | base64)

attrs=$(join_c "icon=$icon" "drag=$drag" "action1=$action" "tooltip=$tooltip" "menu=$menu")
echo -e "\e]515;$attrs;$url\aHello\e]515;;\a"
