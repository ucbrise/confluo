function (create_symlinks)
  # Do nothing if building in-source
  if (${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    return()
  endif()

  foreach (path_file ${ARGN})
    get_filename_component(folder ${path_file} PATH)

    # Create REAL folder
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${folder}")

    # Delete symlink if it exists
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/${path_file}")

    # Get OS dependent path to use in `execute_process`
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${path_file}" link)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${path_file}" target)

    if (UNIX)
      set(command ln -s ${target} ${link})
    else()
      set(command cmd.exe /c mklink ${link} ${target})
    endif()

    execute_process(COMMAND ${command} 
      RESULT_VARIABLE result
      ERROR_VARIABLE output)

    if (NOT ${result} EQUAL 0)
      message(FATAL_ERROR "Could not create symbolic link for: ${target} --> ${output}")
    endif()

  endforeach(path_file)
endfunction(create_symlinks)

# Find if a Python module is installed
# Found at http://www.cmake.org/pipermail/cmake/2011-January/041666.html
# To use do: find_python_module(PyQt4 REQUIRED)
function(find_python_module module)
  string(TOUPPER ${module} module_upper)
  if(NOT PY_${module_upper})
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
      set(${module}_FIND_REQUIRED TRUE)
    endif()
    # A module's location is usually a directory, but for binary modules
    # it's a .so file.
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c" 
      "import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
      RESULT_VARIABLE _${module}_status 
      OUTPUT_VARIABLE _${module}_location
      ERROR_QUIET 
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _${module}_status)
      set(PY_${module_upper} ${_${module}_location} CACHE STRING 
        "Location of Python module ${module}")
    endif(NOT _${module}_status)
  endif(NOT PY_${module_upper})
  find_package_handle_standard_args(${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)