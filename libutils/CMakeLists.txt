include_directories(utils)

add_library(confluoutils STATIC
        utils/byte_utils.h
        utils/function_traits.h
        utils/bit_utils.h
        utils/string_utils.h
        utils/cmd_parse.h
        utils/file_utils.h
        utils/rand_utils.h
        utils/assertions.h
        utils/mmap_utils.h
        utils/optional.h
        utils/thread_utils.h
        utils/config_utils.h
        utils/time_utils.h
        utils/math_utils.h
        utils/io_utils.h
        utils/debug_utils.h
        utils/logger.h
        utils/error_handling.h
        utils/atomic.h
        src/utils/assertions.cc
        src/utils/cmd_parse.cc src/utils/config_utils.cc src/utils/error_handling.cc src/utils/file_utils.cc src/utils/io_utils.cc src/utils/logger.cc src/utils/math_utils.cc src/utils/mmap_utils.cc src/utils/rand_utils.cc src/utils/string_utils.cc src/utils/time_utils.cc)

# install
install(DIRECTORY utils/
        DESTINATION include
        FILES_MATCHING PATTERN "*")
