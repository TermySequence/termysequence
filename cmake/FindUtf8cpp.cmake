INCLUDE(FindPackageHandleStandardArgs)

# Use the bundled copy of UTF8-CPP
SET(Utf8cpp_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/vendor/utf8cpp)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Utf8cpp DEFAULT_MSG Utf8cpp_INCLUDE_DIR)

IF (Utf8cpp_FOUND)
  ADD_LIBRARY(l::Utf8cpp INTERFACE IMPORTED)
  SET_TARGET_PROPERTIES(l::Utf8cpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Utf8cpp_INCLUDE_DIR}"
  )
ENDIF()
