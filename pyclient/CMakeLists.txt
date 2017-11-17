include(FindPythonInterp)

if (PYTHONINTERP_FOUND)
  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
  set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
  configure_file(${SETUP_PY_IN} ${SETUP_PY})

  file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py *.cfg *.xml)
  create_symlinks(${files})

  add_custom_target(pyclient_build ALL
    COMMAND ${PYTHON_EXECUTABLE} setup.py build --build-base ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Python client"
  )
  
  if (BUILD_TESTS)
    add_test(NAME PythonClientTest 
      COMMAND ${CMAKE_COMMAND} -E env DIALOG_SERVER_EXEC=${PROJECT_BINARY_DIR}/bin/confluod ${PYTHON_EXECUTABLE} setup.py test 
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif ()
  
  install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
else ()
  message(WARNING "Skipping Python Client build: Requirement Python >= 2.7 not met")
endif ()
