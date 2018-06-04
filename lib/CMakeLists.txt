# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

FILE(GLOB common_SOURCES *.cpp)

ADD_LIBRARY(common STATIC ${common_SOURCES})
LTO_LIBRARY(common)
TARGET_LINK_LIBRARIES(common l::Utf8cpp l::Uuid)
