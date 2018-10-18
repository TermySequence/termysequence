# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

LIST(APPEND os_GLOBS *.cpp)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  LIST(APPEND os_GLOBS linux/*.cpp)
  FILE(GLOB osc_SOURCES linux/*.c)
ELSE()
  LIST(APPEND os_GLOBS macos/*.cpp)
  FILE(GLOB osc_SOURCES macos/*.c)
ENDIF()

FILE(GLOB os_SOURCES ${os_GLOBS})

ADD_LIBRARY(os STATIC ${os_SOURCES})
LTO_LIBRARY(os)
ADD_LIBRARY(osc STATIC ${osc_SOURCES})
LTO_LIBRARY(osc)
IF (USE_LIBGIT2)
  ADD_LIBRARY(dyngit STATIC dyn/git.c)
  LTO_LIBRARY(dyngit)
ENDIF()

ADD_DEPENDENCIES(osc gitinfo)
TARGET_LINK_LIBRARIES(os osc)
