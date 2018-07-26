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
tarfile=/tmp/${name}.tar
subfile=/tmp/submodule.tar

# Create the uncompressed archive
git archive --prefix=${name}/ -o $tarfile $commit || exit 2

# Fix up the gitinfo.h.in in the archive
tmpdir=/tmp/$name
mkdir -p $tmpdir || exit 3;

if [ "$commit" = 'HEAD' ]; then
    arg=--head
else
    arg="--tag $commit"
fi

./gitinfo.sh ${arg} ${tmpdir}/gitinfo.h.in || exit 4
tar -uf $tarfile -C /tmp ${name}/gitinfo.h.in || exit 5

# Add submodules to the archive
subcommit=$(git ls-tree $commit vendor/v8-linux | awk '{print $3}')
pushd vendor/v8-linux
git archive --prefix=${name}/vendor/v8-linux/ $subcommit > $subfile || exit 6
tar -Af $tarfile $subfile || exit 6
popd

subcommit=$(git ls-tree $commit vendor/termy-emoji | awk '{print $3}')
pushd vendor/termy-emoji
git archive --prefix=${name}/vendor/termy-emoji/ $subcommit > $subfile || exit 7
tar -Af $tarfile $subfile || exit 7
popd

subcommit=$(git ls-tree $commit vendor/termy-icon-theme | awk '{print $3}')
pushd vendor/termy-icon-theme
git archive --prefix=${name}/vendor/termy-icon-theme/ $subcommit > $subfile || exit 8
tar -Af $tarfile $subfile || exit 8
popd

# Compress the archive
xz <$tarfile >${name}.tar.xz || exit 9

# Remove temporary files
rm ${tmpdir}/gitinfo.h.in
rmdir $tmpdir
rm $subfile
rm $tarfile

echo Success
