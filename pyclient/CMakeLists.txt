add_custom_target(pyclient_build ALL
  COMMAND ${PYTHON_EXECUTABLE} setup.py build
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building Python client"
)

add_test(PythonClientTest ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_dialog_client.py
        ${PROJECT_BINARY_DIR}/bin/dialogd)

find_program(PYTHON "python")

install(CODE "execute_process(COMMAND ${PYTHON} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
