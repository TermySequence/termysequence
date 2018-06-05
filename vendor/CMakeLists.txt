# Copyright © 2018 TermySequence LLC
#
# SPDX-License-Identifier: GPL-2.0-only

IF (INSTALL_SHELL_INTEGRATION)
  INSTALL(FILES iTerm2/iterm2_shell_integration.bash
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/profile.d
    RENAME ${ABBREV_NAME}_shell_integration.sh)
ENDIF()
