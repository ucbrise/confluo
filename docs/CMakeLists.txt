# set input and output files
set(DOXYGEN_CPP_IN ${CMAKE_CURRENT_SOURCE_DIR}/DoxyfileCpp.in)
set(DOXYGEN_CPP_OUT ${CMAKE_CURRENT_BINARY_DIR}/DoxyfileCpp)

# request to configure the file
configure_file(${DOXYGEN_CPP_IN} ${DOXYGEN_CPP_OUT} @ONLY)

# set input and output files
set(MKDOCS_IN ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml.in)
set(MKDOCS_OUT ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.yml)

# request to configure the file
configure_file(${MKDOCS_IN} ${MKDOCS_OUT} @ONLY)

# generate text documentation using mkdocs
add_custom_target(doc_mkdocs ALL
  COMMAND ${MKDOCS_EXECUTABLE} build
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating Documentation with MkDocs"
  VERBATIM)

# serve documentation website locally
add_custom_target(doc_serve
  COMMAND ${MKDOCS_EXECUTABLE} serve
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Serving Documentation built by MkDocs"
  VERBATIM)

# generate doxygen API documentation
add_custom_target(doc_doxygen ALL
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CPP_OUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating C++ API documentation with Doxygen"
  VERBATIM)

