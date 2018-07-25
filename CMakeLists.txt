cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(confluo C CXX)

set(CONFLUO_VERSION_MAJOR "0")
set(CONFLUO_VERSION_MINOR "1")
set(CONFLUO_VERSION_PATCH "0")
set(CONFLUO_VERSION "${CONFLUO_VERSION_MAJOR}.${CONFLUO_VERSION_MINOR}.${CONFLUO_VERSION_PATCH}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "Windows build not supported")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(SUPPRESS_WARN_FLAGS 
    "${SUPPRESS_WARN_FLAGS} -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-conversion-null" )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(SUPPRESS_WARN_FLAGS 
    "${SUPPRESS_WARN_FLAGS} -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-private-field -Wno-unused-const-variable")
endif()

# SUPPRESS_WARN_FLAGS must go **after** all flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

set(CMAKE_VERBOSE_MAKEFILE false
    CACHE STRING "Show all outputs including compiler lines." FORCE)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

if(APPLE)
  # disable warnings about "ranlib: file: xxx.a(...cc.o) has no symbols"
  set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
  set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Options
include(BuildOptions)

# Functions
include(Functions)

# Dependencies
include(Dependencies)

# Testing
if (BUILD_TESTS)
  include(CTest)
endif()

# Utilities
add_subdirectory(libutils)

# Core library
add_subdirectory(libconfluo)

if (BUILD_RPC)
  # RPC Framework
  #add_subdirectory(librpc)
  
  # Python Client
  #  if (WITH_PY_CLIENT)
  #    add_subdirectory(pyclient)
  #  endif()
  #  
  #  # Java Client
  #  if (WITH_JAVA_CLIENT)
  #    add_subdirectory(javaclient)
  #  endif()
endif()

# Confluo examples
if (BUILD_EXAMPLES)
    #  add_subdirectory(examples/libtimeseries)
    #  add_subdirectory(examples/libstreaming)
endif()

if (BUILD_DOC)
  add_subdirectory(docs)
endif()

# CPack
set(CPACK_PACKAGE_NAME "libconfluo")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Confluo")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "RISELab")
set(CPACK_PACKAGE_CONTACT "anuragk@berkeley.edu")

set(CPACK_PACKAGE_VERSION ${CONFLUO_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${CONFLUO_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CONFLUO_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CONFLUO_VERSION_PATCH})

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;tags;cscope.*")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

include(CPack)
