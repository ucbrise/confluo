cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(dialog VERSION 0.1.0 LANGUAGES CXX C)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "Windows build not supported")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -pedantic -Wextra -fno-strict-aliasing")
enable_language(C)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  message(STATUS "GNU compiler detected")
  # Suppress thrift warnings
  set(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-maybe-uninitialized")
  # Suppress unnecessary warnings
  set(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wno-conversion-null" )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(STATUS "Apple Clang compiler detected")
  # Suppress thrift warnings
  set(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-private-field -Wno-unused-const-variable")
  # Suppress unnecessary warnings
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

include(CTest)

# Boost
find_package(Boost 1.55.0)

# Google Test
add_subdirectory(external/gtest-1.7.0)

# DiaLog core
add_subdirectory(libdialog)

if(Boost_FOUND)
  # Thrift
  include(FindPythonInterp)
  find_package(Java)
  include(UseJava)
  set(BUILD_COMPILER OFF CACHE BOOL "Build Thrift compiler" FORCE)
  set(WITH_CPP ON CACHE BOOL "Build C++ Thrift library" FORCE)
  set(WITH_C_GLIB OFF CACHE BOOL "Build C (GLib) Thrift library" FORCE)
  set(WITH_JAVA ${Java_FOUND} CACHE BOOL "Build Java Thrift library" FORCE) # For now
  set(WITH_PYTHON ${PYTHONINTERP_FOUND} CACHE BOOL "Build Python Thrift library" FORCE)
  set(WITH_HASKELL OFF CACHE BOOL "Build Haskell Thrift library" FORCE)
  set(WITH_SHARED_LIB ON CACHE BOOL "Build shared libraries" FORCE)
  set(WITH_STATIC_LIB ON CACHE BOOL "Build static libraries" FORCE)
  set(WITH_ZLIB OFF CACHE BOOL "Build with ZLIB support" FORCE) # For now
  set(WITH_QT4 OFF CACHE BOOL "Build with Qt4 support" FORCE)
  set(WITH_QT5 OFF CACHE BOOL "Build with Ot5 support" FORCE)
  set(WITH_OPENSSL OFF CACHE BOOL "Build with OpenSSL support" FORCE) # For now
  set(WITH_LIBEVENT OFF CACHE BOOL "Build with libevent support" FORCE) # For now
  add_subdirectory(external/thrift-0.10.0)

  # RPC
  add_subdirectory(librpc)

  if (PYTHONINTERP_FOUND)
    set(THRIFT_PYDIR ${PROJECT_SOURCE_DIR}/external/thrift-0.10.0/lib/py/)
    set(THRIFT_PYINSTALL "${PYTHON_EXECUTABLE} setup.py install")
    set(THRIFT_PYCMD "COMMAND ${THRIFT_PYINSTALL} WORKING_DIRECTORY ${THRIFT_PYDIR}")
    install(CODE "execute_process(${THRIFT_PYCMD})")
  
    # Python client
    add_subdirectory(pyclient)
  endif()
  
  if (Java_FOUND)
    # Java client
    add_subdirectory(javaclient)
  endif()
else()
  message(WARNING "Boost libraries not found, will skip RPC build")
endif()

# CPack
set(CPACK_PACKAGE_NAME "libdialog")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DiaLog")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "RISELab")
set(CPACK_PACKAGE_CONTACT "anuragk@berkeley.edu")

set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;tags;cscope.*")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

include(CPack)
