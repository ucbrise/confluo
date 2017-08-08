# - Find Thrift (a cross platform RPC lib/tool)
# This module defines
#  THRIFT_VERSION, version string of ant if found
#  THRIFT_INCLUDE_DIR, where to find THRIFT headers
#  THRIFT_LIBS, THRIFT libraries
#  THRIFT_FOUND, If false, do not try to use ant

# prefer the thrift version supplied in THRIFT_HOME
find_path(THRIFT_INCLUDE_DIR thrift/Thrift.h HINTS
  $ENV{THRIFT_HOME}/include/
  /usr/local/include/
  /opt/local/include/
)

set(THRIFT_LIB_PATHS
  $ENV{THRIFT_HOME}/lib
  /usr/local/lib
  /opt/local/lib)

find_path(THRIFT_STATIC_LIB_PATH libthrift.a PATHS ${THRIFT_LIB_PATHS})

# prefer the thrift version supplied in THRIFT_HOME
find_library(THRIFT_LIB NAMES thrift HINTS ${THRIFT_LIB_PATHS})

find_program(THRIFT_COMPILER thrift
  $ENV{THRIFT_HOME}/bin
  /usr/local/bin
  /usr/bin
  NO_DEFAULT_PATH
)

if (THRIFT_LIB)
  set(THRIFT_FOUND TRUE)
  set(THRIFT_LIBS ${THRIFT_LIB})
  set(THRIFT_STATIC_LIB ${THRIFT_STATIC_LIB_PATH}/libthrift.a)
  exec_program(${THRIFT_COMPILER}
    ARGS -version OUTPUT_VARIABLE THRIFT_VERSION RETURN_VALUE THRIFT_RETURN)
  string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+-[a-z]+$" THRIFT_VERSION_STRING ${THRIFT_VERSION})
else ()
  set(THRIFT_FOUND FALSE)
endif ()

if (THRIFT_FOUND)
  if (NOT THRIFT_FIND_QUIETLY)
    message(STATUS "Found ${THRIFT_VERSION}")
  endif ()
else ()
  message(STATUS "Thrift compiler/libraries NOT found.")
endif ()


mark_as_advanced(
  THRIFT_LIB
  THRIFT_COMPILER
  THRIFT_INCLUDE_DIR
)
