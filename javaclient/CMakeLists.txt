set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.7" "-target" "1.7" "-nowarn")

find_package(Java)
find_package(Ant)

if (Java_FOUND AND Ant_FOUND)
  file(GLOB_RECURSE CONFLUO_CLIENT_SOURCES src/main/java/*.java)
  set(JAVA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/java")
  set(JAVA_DOC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${DOC_INSTALL_DIR}/java")
  
  set(CONFLUO_CLIENT_ARTIFACTS confluo-client-0.1.0.jar confluo-client-0.1.0.pom)
  
  add_custom_command(
      OUTPUT ${CONFLUO_CLIENT_ARTIFACTS}
      COMMAND ${Ant_EXECUTABLE} ${ANT_FLAGS} -Dbuild.dir="${CMAKE_CURRENT_BINARY_DIR}" -f build.xml
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/build.xml
      DEPENDS ${CONFLUO_CLIENT_SOURCES}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
      
  add_custom_target(CONFLUO_CLIENT ALL
      COMMENT "Building Confluo Java Client library using Ant"
      DEPENDS ${CONFLUO_CLIENT_ARTIFACTS}
  )
  
  if (BUILD_TESTS)
    # TODO: Add tests
  endif()
else()
  message(WARNING "Skipping Java Client build: requirements Java >= 1.7 and/or Ant >= 1.6.2 not met")
endif()
