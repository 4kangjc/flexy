cmake_minimum_required(VERSION 3.2)
project(flexy)

include (cmake/utils.cmake)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(./)

set(LIB_SRC
    flexy/thread/thread.cpp
    flexy/util/util.cpp
    flexy/util/log.cpp
    flexy/util/file.cpp
    flexy/schedule/fiber.cpp
)

set(LIBS
    flexy
    pthread
    fmt
    boost_context
)

add_library(flexy STATIC ${LIB_SRC})
redefine_file_macro(flexy)

flexy_add_executable(test_thread "tests/test_thread.cc" "${LIBS}")
flexy_add_executable(test_log "tests/test_logger.cc" "${LIBS}")
flexy_add_executable(test_file "tests/test_file.cc" "${LIBS}")
flexy_add_executable(test_fiber "tests/test_fiber.cc" "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
