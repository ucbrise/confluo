include(CMakeDependentOption)

# Components to build
option(BUILD_TESTS "Build with unittests" ON)
option(BUILD_RPC "Build RPC framework" ON)
option(BUILD_DOC "Build documentation" ON)
CMAKE_DEPENDENT_OPTION(WITH_PY_CLIENT "Build python client" ON "BUILD_RPC" OFF)
CMAKE_DEPENDENT_OPTION(WITH_JAVA_CLIENT "Build java client" ON "BUILD_RPC" OFF)

message(STATUS "----------------------------------------------------------")
message(STATUS "DiaLog version:                           ${DIALOG_VERSION}")
message(STATUS "Build configuration Summary")
message(STATUS "  Build RPC Framework:                    ${BUILD_RPC}")
message(STATUS "    Build python client libraries:        ${WITH_PY_CLIENT}")
message(STATUS "    Build java client libraries:          ${WITH_JAVA_CLIENT}")
message(STATUS "  Build with unit tests:                  ${BUILD_TESTS}")
message(STATUS "  Build documentation:                    ${BUILD_DOC}")
message(STATUS "----------------------------------------------------------")
