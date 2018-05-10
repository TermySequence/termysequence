# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

IF (INSTALL_SHELL_INTEGRATION)
  IF (CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    SET(PROFILE_DIR /etc/profile.d)
  ELSE()
    SET(PROFILE_DIR ${CMAKE_INSTALL_PREFIX}/etc/profile.d)
  ENDIF()

  INSTALL(FILES iTerm2/iterm2_shell_integration.bash DESTINATION ${PROFILE_DIR}
    RENAME ${ABBREV_NAME}_shell_integration.sh)
ENDIF()
