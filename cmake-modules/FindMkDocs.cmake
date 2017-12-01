# FindMkDocs
# -----------
#
# This module looks for MkDocs and is based on the FindDoxygen module
#
# MkDocs is a static site generator that's geared towards building project documentation.
# http://www.mkdocs.org
#
# This modules defines the following variables:
#
# ::
#
#    MKDOCS_EXECUTABLE     = The path to the MkDocs command.
#    MKDOCS_FOUND          = Was MkDocs found or not?
#    MKDOCS_VERSION        = The version reported by mkdocs --version
#

#
# Find MkDocs...
#

find_program(MKDOCS_EXECUTABLE
  NAMES mkdocs
  PATHS
    /usr/local/bin
  DOC "MkDocs documentation generation tool (http://www.mkdocs.org)"
)

if(MKDOCS_EXECUTABLE)
  execute_process(COMMAND ${MKDOCS_EXECUTABLE} "--version" OUTPUT_VARIABLE MKDOCS_VERSION_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[0-9]+\\.*" MKDOCS_VERSION ${MKDOCS_VERSION_OUTPUT})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MkDocs DEFAULT_MSG MKDOCS_EXECUTABLE MKDOCS_VERSION)

mark_as_advanced(MKDOCS_EXECUTABLE)
