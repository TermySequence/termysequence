#!/bin/bash

cleanup() {
    popd
}
trap cleanup EXIT

pushd $HOME/build
make -j4
