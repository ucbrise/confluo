include (ExternalProject)

set(GTEST_VERSION "1.8.0")
set(THRIFT_VERSION "0.10.0")

set(EXTERNAL_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}}")
set(EXTERNAL_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ${CMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}}")

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

set(GTEST_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                     -DCMAKE_INSTALL_PREFIX=${GTEST_PREFIX}
                     -Dgtest_force_shared_crt=ON
                     -DCMAKE_CXX_FLAGS=${GTEST_CMAKE_CXX_FLAGS})

ExternalProject_Add(googletest
  URL "https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz"
  BUILD_BYPRODUCTS "${GTEST_STATIC_LIB}" "${GTEST_MAIN_STATIC_LIB}"
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

set(THRIFT_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
set(THRIFT_C_FLAGS "${EXTERNAL_C_FLAGS}")

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.55.0)

if (Boost_FOUND)
  message(STATUS "Boost include dir: " ${Boost_INCLUDE_DIRS})
  message(STATUS "Boost libraries: " ${Boost_LIBRARIES})

  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${Boost_LIBRARIES})

  set(THRIFT_PREFIX "${PROJECT_BINARY_DIR}/external/thrift")
  set(THRIFT_HOME "${THRIFT_PREFIX}")
  set(THRIFT_INCLUDE_DIR "${THRIFT_PREFIX}/include")
  set(ANT_FLAGS "-Dcompilerarg='-Xlint:-unchecked -source 1.7'")
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
                        "-DWITH_STATIC_LIB=ON"
                        )

  set(THRIFT_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}thrift")
  set(THRIFT_STATIC_LIB "${THRIFT_PREFIX}/lib/${THRIFT_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  ExternalProject_Add(thrift
      URL "http://archive.apache.org/dist/thrift/${THRIFT_VERSION}/thrift-${THRIFT_VERSION}.tar.gz"
      BUILD_BYPRODUCTS "${THRIFT_STATIC_LIB}"
      CMAKE_ARGS ${THRIFT_CMAKE_ARGS})

  include_directories(SYSTEM ${THRIFT_INCLUDE_DIR} ${THRIFT_INCLUDE_DIR}/thrift)
  message(STATUS "Thrift include dir: ${THRIFT_INCLUDE_DIR}")
  message(STATUS "Thrift static library: ${THRIFT_STATIC_LIB}")
  message(STATUS "Thrift compiler: ${THRIFT_COMPILER}")
  add_library(thriftstatic STATIC IMPORTED GLOBAL)
  set_target_properties(thriftstatic PROPERTIES IMPORTED_LOCATION ${THRIFT_STATIC_LIB})
  set(THRIFT_INSTALLED TRUE PARENT_SCOPE)
else()
  set(THRIFT_INSTALLED FALSE PARENT_SCOPE)
  message(WARNING "Boost libraries not found, will skip RPC build")
endif()