#!/bin/bash
# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

#
## Generates a distribution tarball, substituting gitinfo.h.in appropriately
## Run me from the toplevel git folder
#

# Optional argument is the tag name to package
commit=${1:-HEAD}

if [ ! -d '.git' ]; then
    echo "Git folder not present!" 1>&2
    exit 1
fi

package=$(awk '/^PackageName:/ {print $2}' LICENSE.spdx)
version=$(awk '/^PackageVersion:/ {print $2}' LICENSE.spdx)
name="${package}-${version}"

# Create the uncompressed archive
git archive --prefix=${name}/ -o /tmp/${name}.tar $commit || exit 2

# Fix up the gitinfo.h.in in the archive
tmpdir=/tmp/$name
mkdir -p $tmpdir || exit 3;

if [ "$commit" = 'HEAD' ]; then
    arg=--head
else
    arg="--tag $commit"
fi

./gitinfo.sh ${arg} ${tmpdir}/gitinfo.h.in || exit 4
tar -uf /tmp/${name}.tar -C /tmp ${name}/gitinfo.h.in || exit 5

# Compress the archive
xz </tmp/${name}.tar >${name}.tar.xz || exit 6

rm ${tmpdir}/gitinfo.h.in
rmdir $tmpdir;
rm /tmp/${name}.tar

echo Success
