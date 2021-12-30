#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cglm::cglm" for configuration "Debug"
set_property(TARGET cglm::cglm APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(cglm::cglm PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libcglm.so.0.8.5"
  IMPORTED_SONAME_DEBUG "libcglm.so.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS cglm::cglm )
list(APPEND _IMPORT_CHECK_FILES_FOR_cglm::cglm "${_IMPORT_PREFIX}/lib/libcglm.so.0.8.5" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
