#!/bin/bash

if [ -z "$DISPLAY" ]; then
    echo DISPLAY! 1>&2
    exit 1
fi

qtermyd "$@" 2>&1 | perl -pe '$_ = "" if m/^\s*$/; $_ = "" if m/warning/i'
