include(FindPythonInterp)

if (PYTHONINTERP_FOUND)
  add_custom_target(pyclient_build ALL
    COMMAND ${PYTHON_EXECUTABLE} setup.py build --build-base ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Python client"
  )
  
  if (BUILD_TESTS)
    add_test(NAME PythonClientTest 
      COMMAND ${CMAKE_COMMAND} -E env DIALOG_SERVER_EXEC=${PROJECT_BINARY_DIR}/bin/dialogd ${PYTHON_EXECUTABLE} setup.py test 
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif ()
  
  find_program(PYTHON "python")
  
  install(CODE "execute_process(COMMAND ${PYTHON} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
else ()
  message(WARNING "Skipping Python Client build: Requirement Python >= 2.7 not met")
endif ()