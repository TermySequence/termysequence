#!/bin/bash
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

#
## Generates a header file containing the current git revision
## Usage: [--head|--tag <tag>|--none] <outputfile>
#

TEMPLATE=gitinfo.h.in

if [ ! -f $TEMPLATE ]; then
    echo "Error: input file $TEMPLATE not found" 1>&2
    exit 2
fi

OUTFILE=${@: -1}
TMPFILE=$(mktemp)

if [ "$1" = "--none" ]; then
    cp -f "$TEMPLATE" "$TMPFILE" || exit 3
    mv -f "$TMPFILE" "$OUTFILE" || exit 4
    exit 0
fi

echo "Updating $OUTFILE with new git info"

if [ "$1" = "--tag" ]; then
    GITREV=$(git rev-parse $2)
    if [ $? -ne 0 ]; then exit $?; fi
    GITDESC=$2
else
    GITREV=$(git rev-parse HEAD)
    if [ $? -ne 0 ]; then exit $?; fi
    GITDESC=$(git describe --dirty --always)
    if [ $? -ne 0 ]; then exit $?; fi
fi

sed -e "s/GITREV .*/GITREV \"${GITREV}\"/" \
    -e "s/GITDESC .*/GITDESC \"${GITDESC}\"/" \
    "$TEMPLATE" > "$TMPFILE" || exit 3
mv -f "$TMPFILE" "$OUTFILE" || exit 4

exit 0
