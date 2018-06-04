# set input and output files
set(DOXYGEN_CPP_IN ${CMAKE_CURRENT_SOURCE_DIR}/DoxyfileCpp.in)
set(DOXYGEN_CPP_OUT ${CMAKE_CURRENT_BINARY_DIR}/DoxyfileCpp)

# request to configure the file
configure_file(${DOXYGEN_CPP_IN} ${DOXYGEN_CPP_OUT} @ONLY)

# set input and output files
set(DOXYGEN_CPP_CLIENT_IN ${CMAKE_CURRENT_SOURCE_DIR}/DoxyfileCppClient.in)
set(DOXYGEN_CPP_CLIENT_OUT ${CMAKE_CURRENT_BINARY_DIR}/DoxyfileCppClient)

# request to configure the file
configure_file(${DOXYGEN_CPP_CLIENT_IN} ${DOXYGEN_CPP_CLIENT_OUT} @ONLY)

# set input and output files
set(MKDOCS_IN ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml.in)
set(MKDOCS_OUT ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.yml)

# request to configure the file
configure_file(${MKDOCS_IN} ${MKDOCS_OUT} @ONLY)

# generate text documentation using mkdocs
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp
  DEPENDS ${MKDOCS_IN} ${CMAKE_CURRENT_SOURCE_DIR}/src/*.md
  COMMAND ${MKDOCS_EXECUTABLE} build
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating Documentation with MkDocs"
  VERBATIM)

add_custom_target(mkdocs ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp)

# generate doxygen API documentation
file(GLOB_RECURSE CPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../libconfluo/confluo/*.h)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp.log
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp ${DOXYGEN_CPP_IN} ${CPP_SOURCES}
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CPP_OUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating C++ API documentation with Doxygen"
  VERBATIM)

add_custom_target(doxygen_cpp ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cpp.log)

file(GLOB_RECURSE CPP_CLIENT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../librpc/rpc/*.h)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_client.log
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp ${DOXYGEN_CPP_CLIENT_IN} ${CPP_CLIENT_SOURCES}
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CPP_CLIENT_OUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating C++ Client API documentation with Doxygen"
  VERBATIM)

add_custom_target(doxygen_cpp_client ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cpp_client.log)

set(PY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../docs/pyclient/source)
set(PY_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build/docs/site/python_client_api)
file(GLOB PY_DOC_SOURCES ${PY_SOURCE_DIR}/*.*)
file(GLOB_RECURSE PY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../pyclient/confluo/*.py)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pyclient.stamp
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp ${PY_DOC_SOURCES} ${PY_SOURCES}
  COMMAND sphinx-build -b html ${PY_SRC_DIR} ${PY_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/pyclient.stamp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating Python Client API documentation with Sphinx"
  VERBATIM
)

add_custom_target(pyclient_doc ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pyclient.stamp
)

set(JAVA_DOC_DIR ${CMAKE_CURRENT_BINARY_DIR}/site/java_client_api)
set(JAVA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../javaclient/src/main/java)
file(GLOB JAVA_SOURCES ${JAVA_SRC_DIR}/*.java)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/javaclient.stamp
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp ${JAVA_SOURCES} javaclient
  COMMAND javadoc -sourcepath ${JAVA_SRC_DIR} -d ${JAVA_DOC_DIR} -subpackages confluo.rpc:org.apache.thrift -classpath ${CMAKE_CURRENT_BINARY_DIR}/../javaclient/target/confluo-client-0.1.0.jar
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/javaclient.stamp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating Java Client API documentation with Javadoc"
  VERBATIM)

add_custom_target(javaclient_doc ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/javaclient.stamp
)

# serve documentation website locally
add_custom_target(docs_serve
  COMMAND ${MKDOCS_EXECUTABLE} serve
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.stamp ${CMAKE_CURRENT_BINARY_DIR}/cpp.log ${CMAKE_CURRENT_BINARY_DIR}/cpp_client.log ${CMAKE_CURRENT_BINARY_DIR}/pyclient.stamp ${CMAKE_CURRENT_BINARY_DIR}/javaclient.stamp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Serving Documentation built by MkDocs"
  VERBATIM)
