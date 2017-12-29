include (ExternalProject)

set(GTEST_VERSION "1.8.0")
set(THRIFT_VERSION "0.10.0")
set(BOOST_VERSION "1.53")
set(DOXYGEN_VERSION "1.8")

find_package(Threads REQUIRED)
find_package(Boost ${BOOST_VERSION} REQUIRED)
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")

set(EXTERNAL_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}}")
set(EXTERNAL_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ${CMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}}")

# Google Test framework
if (BUILD_TESTS)
  if(APPLE)
    set(GTEST_CMAKE_CXX_FLAGS "${EXTERNAL_CXX_FLAGS} -DGTEST_USE_OWN_TR1_TUPLE=1 -Wno-unused-value -Wno-ignored-attributes")
  else()
    set(GTEST_CMAKE_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  endif()
  
  set(GTEST_PREFIX "${PROJECT_BINARY_DIR}/external/gtest")
  set(GTEST_INCLUDE_DIR "${GTEST_PREFIX}/include")
  set(GTEST_STATIC_LIB
    "${GTEST_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(GTEST_MAIN_STATIC_LIB
    "${GTEST_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}")
  
  set(GTEST_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                       "-DCMAKE_INSTALL_PREFIX=${GTEST_PREFIX}"
                       "-Dgtest_force_shared_crt=ON"
                       "-DCMAKE_CXX_FLAGS=${GTEST_CMAKE_CXX_FLAGS}")
  
  ExternalProject_Add(googletest
    URL "https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz"
    CMAKE_ARGS ${GTEST_CMAKE_ARGS})
  
  message(STATUS "GTest include dir: ${GTEST_INCLUDE_DIR}")
  message(STATUS "GTest static library: ${GTEST_STATIC_LIB}")
  message(STATUS "GTest main static library: ${GTEST_MAIN_STATIC_LIB}")
  include_directories(SYSTEM ${GTEST_INCLUDE_DIR})
  
  add_library(gtest STATIC IMPORTED GLOBAL)
  set_target_properties(gtest PROPERTIES IMPORTED_LOCATION ${GTEST_STATIC_LIB})
  
  add_library(gtest_main STATIC IMPORTED GLOBAL)
  set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION
    ${GTEST_MAIN_STATIC_LIB})
endif()

if (BUILD_RPC)
  set(THRIFT_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(THRIFT_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(THRIFT_PREFIX "${PROJECT_BINARY_DIR}/external/thrift")
  set(THRIFT_HOME "${THRIFT_PREFIX}")
  set(THRIFT_INCLUDE_DIR "${THRIFT_PREFIX}/include")
  set(THRIFT_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                        "-DCMAKE_CXX_FLAGS=${THRIFT_CXX_FLAGS}"
                        "-DCMAKE_C_FLAGS=${THRIFT_C_FLAGS}"
                        "-DCMAKE_INSTALL_PREFIX=${THRIFT_PREFIX}"
                        "-DCMAKE_INSTALL_RPATH=${THRIFT_PREFIX}/lib"
                        "-DBUILD_COMPILER=OFF"
                        "-DBUILD_TESTING=OFF"
                        "-DWITH_SHARED_LIB=OFF"
                        "-DWITH_QT4=OFF"
                        "-DWITH_QT5=OFF"
                        "-DWITH_C_GLIB=OFF"
                        "-DWITH_HASKELL=OFF"
                        "-DWITH_ZLIB=OFF" # For now
                        "-DWITH_OPENSSL=OFF" # For now
                        "-DWITH_LIBEVENT=OFF" # For now
                        "-DWITH_JAVA=OFF"
                        "-DWITH_PYTHON=OFF"
                        "-DWITH_CPP=ON"
                        "-DWITH_STDTHREADS=OFF"
                        "-DWITH_BOOSTTHREADS=OFF"
                        "-DWITH_STATIC_LIB=ON"
                        )

  set(THRIFT_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}thrift")
  set(THRIFT_STATIC_LIB "${THRIFT_PREFIX}/lib/${THRIFT_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  ExternalProject_Add(thrift
      URL "http://archive.apache.org/dist/thrift/${THRIFT_VERSION}/thrift-${THRIFT_VERSION}.tar.gz"
      CMAKE_ARGS ${THRIFT_CMAKE_ARGS})

  include_directories(SYSTEM ${THRIFT_INCLUDE_DIR} ${THRIFT_INCLUDE_DIR}/thrift)
  message(STATUS "Thrift include dir: ${THRIFT_INCLUDE_DIR}")
  message(STATUS "Thrift static library: ${THRIFT_STATIC_LIB}")
  add_library(thriftstatic STATIC IMPORTED GLOBAL)
  set_target_properties(thriftstatic PROPERTIES IMPORTED_LOCATION ${THRIFT_STATIC_LIB})
  
  install(FILES ${THRIFT_STATIC_LIB} DESTINATION lib)
  install(DIRECTORY ${THRIFT_INCLUDE_DIR}/thrift DESTINATION include)
endif()

if (WITH_PY_CLIENT)
  include(FindPythonInterp)
  if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Cannot build python client without python interpretor")
  endif()
  find_python_module(setuptools REQUIRED)
  if (NOT PY_SETUPTOOLS)
    message(FATAL_ERROR "Python setuptools is required for python client")
  endif()
endif()

if (WITH_JAVA_CLIENT)
  find_package(Java REQUIRED)
  find_package(Ant REQUIRED)
  set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.7" "-target" "1.7" "-nowarn")
endif()

if (BUILD_DOC)
  find_package(MkDocs REQUIRED)
  find_package(Doxygen ${DOXYGEN_VERSION} REQUIRED)
endif()
