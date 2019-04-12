# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

FIND_PACKAGE(Cmocka 1.1.1 REQUIRED)

ADD_LIBRARY(mockemulator STATIC mockemulator.cpp)
TARGET_LINK_LIBRARIES(mockemulator os common Threads::Threads)

# Informal testing tools
IF (MAINTAINER_MODE)
  ADD_EXECUTABLE(recalcitrant recalcitrant.c)
  ADD_EXECUTABLE(slowreader slowreader.c)
  ADD_EXECUTABLE(mousetest mousetest.c)
  ADD_EXECUTABLE(testtool testtool.cpp)
  ADD_EXECUTABLE(unidraw unidraw.cpp)
  TARGET_LINK_LIBRARIES(unidraw l::Utf8cpp)
  ADD_EXECUTABLE(attrparser attrparser.cpp)
  TARGET_LINK_LIBRARIES(attrparser os common)
  ADD_EXECUTABLE(montecarlo montecarlo.cpp)
  TARGET_LINK_LIBRARIES(montecarlo mockemulator)
ENDIF()

# Formal unit tests
FUNCTION(DEFTEST name)
  ADD_EXECUTABLE(${name} ${name}.cpp)
  ADD_TEST(${name} ${name})
  TARGET_LINK_LIBRARIES(${name} mockemulator l::Cmocka)
ENDFUNCTION()

DEFTEST(simpleupdate)
DEFTEST(fillreplace)
DEFTEST(splitchar)
DEFTEST(removechar)
DEFTEST(mergechars)
DEFTEST(substr)
DEFTEST(remove)
DEFTEST(insert)
DEFTEST(replace)
DEFTEST(erase)
DEFTEST(eraserange)
