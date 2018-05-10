# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -ffat-lto-objects")
  SET(CMAKE_AR "gcc-ar")
  SET(CMAKE_RANLIB "gcc-ranlib")
ENDIF()

FILE(GLOB common_SOURCES *.cpp)

ADD_LIBRARY(common STATIC ${common_SOURCES})
TARGET_LINK_LIBRARIES(common l::Utf8cpp l::Uuid)
