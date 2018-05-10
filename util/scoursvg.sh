#!/bin/bash

if [ "$1" = "-a" ]; then
    files=$(find src/dist/images -name \*.svg)
elif [ "$1" ]; then
    files="$*"
else
    files=$(git ls-files -m src/dist/images)
fi

for file in $files; do
    if [[ $(wc -c $file | awk '{print $1}') -ge 256 ]]; then
        rm -f /tmp/scratch.svg
        scour -i $file -o /tmp/scratch.svg
        mv -f /tmp/scratch.svg $file
    else
        echo Skipping $file
    fi
done
