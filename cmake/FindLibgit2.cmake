INCLUDE(FindPackageHandleStandardArgs)

PKG_CHECK_MODULES(PC_Libgit2 QUIET libgit2)

FIND_LIBRARY(Libgit2_LIBRARY
  NAMES git2
  PATHS ${PC_Libgit2_LIBRARY_DIRS}
)
FIND_PATH(Libgit2_INCLUDE_DIR
  NAMES git2.h
  PATHS ${PC_Libgit2_INCLUDE_DIRS}
)

SET(Libgit2_VERSION ${PC_Libgit2_VERSION})
IF (NOT Libgit2_VERSION AND Libgit2_INCLUDE_DIR AND EXISTS "${Libgit2_INCLUDE_DIR}/git2/version.h")
  FILE(STRINGS "${Libgit2_INCLUDE_DIR}/git2/version.h" _VERS_H REGEX "#define LIBGIT2_VERSION +\"[0-9].+\"")
  STRING(REGEX REPLACE ".*\"([0-9][^ ]+)\".*" "\\1" Libgit2_VERSION "${_VERS_H}")
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libgit2
  REQUIRED_VARS Libgit2_LIBRARY Libgit2_INCLUDE_DIR
  VERSION_VAR Libgit2_VERSION
)

IF (Libgit2_FOUND)
  ADD_LIBRARY(l::Libgit2 UNKNOWN IMPORTED)
  SET_TARGET_PROPERTIES(l::Libgit2 PROPERTIES
    IMPORTED_LOCATION "${Libgit2_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_Libgit2_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${Libgit2_INCLUDE_DIR}"
  )
ENDIF()
