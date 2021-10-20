cmake_minimum_required(VERSION 3.2)
project(flexy)

include (cmake/utils.cmake)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(./)

set(LIB_SRC
    flexy/thread/thread.cpp
    flexy/util/util.cpp
    flexy/util/log.cpp
    flexy/util/file.cpp
    flexy/schedule/fiber.cpp
    flexy/schedule/scheduler.cpp
    flexy/util/config.cpp
    flexy/schedule/timer.cpp
    flexy/schedule/channel.cpp
    flexy/schedule/iomanager.cpp
    flexy/net/fd_manager.cpp
    flexy/net/hook.cpp
    flexy/net/address.cpp
)

set(LIBS
    flexy
    pthread
    fmt
    boost_context
    yaml-cpp
    jsoncpp
    dl
)

option(YAML "Config by Json" ON)
option(JSON "Config by Json" OFF)
option(BUILD_SHARED_LIBS "Build flexy as a shared lib" ON)
option(BUILD_STATIC_LIBS "Build flext as a static lib" OFF)

if(JSON)
    add_definitions(-DFLEXY_JSON)
elseif(YAML)
    add_definitions(-DFLEXY_YAML)
endif(JSON)

if (BUILD_STATIC_LIBS)
add_library(flexy STATIC ${LIB_SRC})
elseif (BUILD_SHARED_LIBS)
add_library(flexy SHARED ${LIB_SRC})
endif(BUILD_STATIC_LIBS)

redefine_file_macro(flexy)

flexy_add_executable(test_thread "tests/test_thread.cc" "${LIBS}")
flexy_add_executable(test_log "tests/test_logger.cc" "${LIBS}")
flexy_add_executable(test_file "tests/test_file.cc" "${LIBS}")
flexy_add_executable(test_fiber "tests/test_fiber.cc" "${LIBS}")
flexy_add_executable(test_scheduler "tests/test_scheduler.cc" "${LIBS}")
flexy_add_executable(test_task "tests/test_task.cc" "${LIBS}")
flexy_add_executable(test_config "tests/test_config.cc" "${LIBS}")
flexy_add_executable(test_timer "tests/test_timer.cc" "${LIBS}")
flexy_add_executable(test_iomanager "tests/test_iomanager.cc" "${LIBS}")
flexy_add_executable(test_hook "tests/test_hook.cc" "${LIBS}")
flexy_add_executable(test_address "tests/test_address.cc" "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
