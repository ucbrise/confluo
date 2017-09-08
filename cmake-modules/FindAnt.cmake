#  ANT_FOUND - system has Ant
#  Ant_EXECUTABLE - the Ant executable
#
# It will search the environment variable ANT_HOME if it is set

include(FindPackageHandleStandardArgs)

find_program(Ant_EXECUTABLE NAMES ant PATHS $ENV{ANT_HOME}/bin)
find_package_handle_standard_args(Ant DEFAULT_MSG Ant_EXECUTABLE)
mark_as_advanced(Ant_EXECUTABLE)
