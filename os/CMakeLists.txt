# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto -ffat-lto-objects")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -ffat-lto-objects")
  SET(CMAKE_AR "gcc-ar")
  SET(CMAKE_RANLIB "gcc-ranlib")
ENDIF()

LIST(APPEND os_GLOBS *.cpp)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  LIST(APPEND os_GLOBS linux/*.cpp)
  FILE(GLOB osc_SOURCES linux/*.c)
ELSE()
  LIST(APPEND os_GLOBS macos/*.cpp)
  FILE(GLOB osc_SOURCES macos/*.c)
ENDIF()

IF (MEMDEBUG)
  LIST(APPEND os_GLOBS memdbg/memdbg.cpp)
ENDIF()

FILE(GLOB os_SOURCES ${os_GLOBS})

ADD_LIBRARY(os STATIC ${os_SOURCES})
ADD_LIBRARY(osc STATIC ${osc_SOURCES})
IF (USE_LIBGIT2)
  ADD_LIBRARY(dyngit STATIC dyn/git.c)
ENDIF()

ADD_DEPENDENCIES(osc gitinfo)
TARGET_LINK_LIBRARIES(os osc)
