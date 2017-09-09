cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(dialog CXX C)

set(DIALOG_VERSION_MAJOR "0")
set(DIALOG_VERSION_MINOR "1")
set(DIALOG_VERSION_PATCH "0")
set(DIALOG_VERSION "${DIALOG_VERSION_MAJOR}.${DIALOG_VERSION_MINOR}.${DIALOG_VERSION_PATCH}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "Windows build not supported")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(FATAL_ERROR "${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()
enable_language(C)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(SUPPRESS_WARN_FLAGS 
    "${SUPPRESS_WARN_FLAGS} -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-conversion-null" )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(SUPPRESS_WARN_FLAGS 
    "${SUPPRESS_WARN_FLAGS} -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-private-field -Wno-unused-const-variable")
endif()

# SUPPRESS_WARN_FLAGS must go **after** all flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -pedantic -Wextra ${SUPPRESS_WARN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

set(CMAKE_BUILD_TYPE RelWithDebInfo)

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

# Dependencies
include(Dependencies)

# Testing
if (BUILD_TESTS)
  include(CTest)
  set(TEST_LINK_LIBS gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
endif()

# DiaLog utils
add_subdirectory(libutils)

# DiaLog core
add_subdirectory(libdialog)

if (BUILD_RPC)
  # RPC Framework
  add_subdirectory(librpc)
  
  # Python Client
  if (WITH_PY_CLIENT)
    add_subdirectory(pyclient)
  endif()
  
  # Java Client
  if (WITH_JAVA_CLIENT)
    add_subdirectory(javaclient)
  endif()
endif()

# CPack
set(CPACK_PACKAGE_NAME "libdialog")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DiaLog")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "RISELab")
set(CPACK_PACKAGE_CONTACT "anuragk@berkeley.edu")

set(CPACK_PACKAGE_VERSION ${DIALOG_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${DIALOG_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DIALOG_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DIALOG_VERSION_PATCH})

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;tags;cscope.*")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

include(CPack)
