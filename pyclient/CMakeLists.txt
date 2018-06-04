set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
configure_file(${SETUP_PY_IN} ${SETUP_PY})

file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py *.cfg *.xml)
create_symlinks(${files})

add_custom_target(pyclient ALL
  COMMAND ${PYTHON_EXECUTABLE} setup.py build --build-base ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Building Python client"
)
  
if (BUILD_TESTS)
  set(SERVER_EXEC "${CMAKE_CURRENT_BINARY_DIR}/../librpc/confluod")
  add_test(NAME PythonClientTest 
    COMMAND ${CMAKE_COMMAND} -E env CONFLUO_SERVER_EXEC=${SERVER_EXEC} ${PYTHON_EXECUTABLE} setup.py test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif ()
  
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
