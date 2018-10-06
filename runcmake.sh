#!/bin/bash
declare -a args

# Location where build output will go:
BUILD_FOLDER=${1:-./build}

# Build Profile (see case statement below for valid values):
BUILD_PROFILE=${2:-client+server}

# Build Type (Debug, Release, or another cmake build type):
BUILD_TYPE=${3:-Release}

# More customization options:
#args+=("-DUSE_FUSE3=OFF")
#args+=("-DUSE_FUSE2=ON")
#args+=("-DUSE_SYSTEMD=OFF")
#args+=("-DUSE_LIBGIT2=OFF")
#args+=("-DCMAKE_INSTALL_PREFIX=/usr/local")

# Generator
GENERATOR="Unix Makefiles"

usage () {
    echo "Usage: ./runcmake.sh [BuildFolder] [BuildProfile] [BuildType]" 1>&2
    echo "  BuildProfile: [client+server|server|client|maintainer]" 1>&2
    echo "  BuildType:    [Release|Debug|RelWithDebInfo]" 1>&2
    exit 1
}

if [[ "$BUILD_FOLDER" == -* ]]; then usage; fi
if [ ! -f runcmake.sh ]; then
    echo "Please run me from the main source folder" 1>&2
    exit 2
fi

mkdir -p "$BUILD_FOLDER" || exit 3
cd "$BUILD_FOLDER" || exit 3

case $BUILD_PROFILE in
    client+server)
        ;;
    server)
        args+=("-DBUILD_QTGUI=OFF")
        ;;
    client)
        args+=("-DBUILD_SERVER=OFF")
        ;;
    maintainer)
        args+=("-DMAINTAINER_MODE=ON")
        args+=("-DBUILD_TESTS=ON")
        args+=("-Wdev")
        ;;
    *)
        usage
        ;;
esac

echo "Build profile is: " $BUILD_PROFILE
echo "Build type is:    " $BUILD_TYPE
echo "Build folder is:  " $BUILD_FOLDER

args+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")

if [ -d "$QT5_CMAKE_DIR" ]; then
    args+=("-DQt5_DIR=$QT5_CMAKE_DIR")
fi

echo Running: cmake
for arg in "${args[@]}"; do echo -e "\t$arg"; done
echo -e "\t-G '$GENERATOR'"
echo -e "\t$OLDPWD"
echo

cmake "${args[@]}" -G "$GENERATOR" "$OLDPWD" || exit 4

echo Success
if [ "$GENERATOR" = "Unix Makefiles" ]; then
    echo Now cd to $BUILD_FOLDER and run \"make -j4\"
fi
