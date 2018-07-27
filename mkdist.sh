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

set -e

package=$(awk '/^PackageName:/ {print $2}' LICENSE.spdx)
version=$(awk '/^PackageVersion:/ {print $2}' LICENSE.spdx)
name="${package}-${version}"
servername="${package}-server-${version}"
qtname="${package}-qt-${version}"
tarfile=/tmp/${name}.tar
subfile=/tmp/submodule.tar

# CMake option patterns to remove from each archive
declare -a all_remove
all_remove+=('OPTION.MAINTAINER_MODE')
all_remove+=('OPTION.MEMDEBUG')
declare -a server_remove=("${all_remove[@]}")
server_remove+=('OPTION.BUILD_QTGUI')
server_remove+=('OPTION.USE_FUSE')
server_remove+=('OPTION.NATIVE_DIALOGS')
server_remove+=('SET.V8_')
declare -a qt_remove=("${all_remove[@]}")
qt_remove+=('OPTION.BUILD_SERVER')
qt_remove+=('OPTION.BUILD_TESTS')
qt_remove+=('OPTION.USE_LIBGIT2')
qt_remove+=('OPTION.INSTALL_SHELL_INTEGRATION')

# Create the combined archive
git archive --prefix=${name}/ -o $tarfile $commit

# Fix up files in the archive
tmpdir=/tmp/$name
mkdir -p $tmpdir;

if [ "$commit" = 'HEAD' ]; then
    arg=--head
else
    arg="--tag $commit"
fi
./gitinfo.sh ${arg} ${tmpdir}/gitinfo.h.in

cp CMakeLists.txt $tmpdir
for pat in "${all_remove[@]}"; do
    sed -i "/^${pat}/d" $tmpdir/CMakeLists.txt
done
tar -uf $tarfile -C /tmp ${name}/gitinfo.h.in ${name}/CMakeLists.txt

# Add submodules to the archive
subcommit=$(git ls-tree $commit vendor/v8-linux | awk '{print $3}')
pushd vendor/v8-linux
git archive --prefix=${name}/vendor/v8-linux/ $subcommit > $subfile
tar -Af $tarfile $subfile
popd

subcommit=$(git ls-tree $commit vendor/termy-emoji | awk '{print $3}')
pushd vendor/termy-emoji
git archive --prefix=${name}/vendor/termy-emoji/ $subcommit > $subfile
tar -Af $tarfile $subfile
popd

subcommit=$(git ls-tree $commit vendor/termy-icon-theme | awk '{print $3}')
pushd vendor/termy-icon-theme
git archive --prefix=${name}/vendor/termy-icon-theme/ $subcommit > $subfile
tar -Af $tarfile $subfile
popd

# Compress the archive
echo -n Compressing main archive...
xz <$tarfile >${name}.tar.xz
echo done

# Create the server-only archive
tar --delete \
    ${name}/vendor/v8-linux \
    ${name}/vendor/termy-emoji \
    ${name}/vendor/termy-icon-theme \
    ${name}/src \
    <$tarfile >$subfile

cp CMakeLists.txt $tmpdir
for pat in "${server_remove[@]}"; do
    sed -i "/^${pat}/d" $tmpdir/CMakeLists.txt
done
tar -uf $subfile -C /tmp ${name}/CMakeLists.txt

echo -n Compressing server archive...
xz <$subfile >${servername}.tar.xz
echo done

# Create the qt client-only archive
tar --delete \
    ${name}/mux \
    ${name}/mon \
    ${name}/test \
    <$tarfile >$subfile

cp CMakeLists.txt $tmpdir
for pat in "${qt_remove[@]}"; do
    sed -i "/^${pat}/d" $tmpdir/CMakeLists.txt
done
tar -uf $subfile -C /tmp ${name}/CMakeLists.txt

echo -n Compressing qt archive...
xz <$subfile >${qtname}.tar.xz
echo done

# Remove temporary files
rm -r ${tmpdir}
rm $subfile
rm $tarfile

echo Success
