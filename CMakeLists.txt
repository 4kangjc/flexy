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
    flexy/net/socket.cpp
    flexy/net/tcp_server.cpp
    flexy/env/signal.cpp
    flexy/http/http.cpp
    flexy/http/http_parser.cpp
    flexy/stream/stream.cpp
    flexy/stream/socket_stream.cpp
    flexy/http/http_session.cpp
    flexy/http/http_server.cpp
    flexy/http/servlet.cpp
    flexy/env/env.cpp
    flexy/env/daemon.cpp
    flexy/env/application.cpp
    flexy/schedule/semaphore.cpp 
    flexy/schedule/async_io.cpp
    flexy/db/mysql.cpp
    flexy/db/sqlite3.cpp
    flexy/stream/async_socket_stream.cpp
    flexy/util/hash_util.cpp
    flexy/schedule/worker.cpp
    flexy/http/ws_session.cpp
    flexy/http/ws_servlet.cpp
    flexy/http/ws_server.cpp
    flexy/http2/dynamic_table.cpp
    flexy/schedule/mutex.cpp 
    flexy/net/bytearray.cpp
    flexy/http2/huffman.cpp
    flexy/http2/frame.cpp
    flexy/http2/hpack.cpp
    # flexy/http2/stream.cpp 
    # flexy/http2/http2_stream.cpp 
    # flexy/http2/http2_server.cpp
    flexy/schedule/condition_variable.cpp
)

ragelmaker(flexy/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/flexy/http)
ragelmaker(flexy/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/flexy/http)

set(LIBS
    flexy
    pthread
    fmt
    boost_context
    yaml-cpp
    jsoncpp
    dl
    mysqlclient
    sqlite3
    mbedtls
)

option(YAML "Config by Yaml" ON)
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
target_link_libraries(${LIBS})
elseif (BUILD_SHARED_LIBS)
add_library(flexy SHARED ${LIB_SRC})
target_link_libraries(${LIBS})
endif(BUILD_STATIC_LIBS)

redefine_file_macro(flexy)

option(TESTS "flexy tests execute" OFF)

if(TESTS)

set (TEST_LIBS ${LIBS} gtest)

flexy_add_executable(test_thread "tests/test_thread.cc" "${LIBS}")
flexy_add_executable(test_log "tests/test_logger.cc" "${LIBS}")
flexy_add_executable(test_file "tests/test_file.cc" "${LIBS}")
flexy_add_executable(test_fiber "tests/test_fiber.cc" "${LIBS}")
flexy_add_executable(test_scheduler "tests/test_scheduler.cc" "${LIBS}")
flexy_add_executable(test_task "tests/test_task.cc" "${TEST_LIBS}")
flexy_add_executable(test_config "tests/test_config.cc" "${LIBS}")
flexy_add_executable(test_timer "tests/test_timer.cc" "${LIBS}")
flexy_add_executable(test_iomanager "tests/test_iomanager.cc" "${LIBS}")
flexy_add_executable(test_hook "tests/test_hook.cc" "${LIBS}")
flexy_add_executable(test_address "tests/test_address.cc" "${LIBS}")
flexy_add_executable(test_socket "tests/test_socket.cc" "${LIBS}")
flexy_add_executable(test_tcp_server "tests/test_tcp_server.cc" "${LIBS}")
flexy_add_executable(test_signal "tests/test_signal.cc" "${LIBS}")
flexy_add_executable(test_go "tests/test_go.cc" "${LIBS}")
flexy_add_executable(test_logconf "tests/test_logconfig.cc" "${LIBS}")
flexy_add_executable(test_http "tests/test_http.cc" "${LIBS}")
flexy_add_executable(test_http_parser "tests/test_http_parser.cc" "${LIBS}")
flexy_add_executable(test_http_session "tests/test_http_session.cc" "${LIBS}")
flexy_add_executable(test_env "tests/test_env.cc" "${LIBS}")
flexy_add_executable(test_deamon "tests/test_daemon.cc" "${LIBS}")
flexy_add_executable(test_application "tests/test_application.cc" "${LIBS}")

flexy_add_executable(test_async_io "tests/test_async_io.cc" "${LIBS}")
flexy_add_executable(test_mysql "tests/test_mysql.cc" "${LIBS}")
flexy_add_executable(test_sqlite3 "tests/test_sqlite3.cc" "${LIBS}")

flexy_add_executable(test_worker "tests/test_worker.cc" "${LIBS}")

flexy_add_executable(test_hash_util "tests/test_hash_util.cc" "${LIBS}")
flexy_add_executable(test_ws_server "tests/test_ws_server.cc" "${LIBS}")

flexy_add_executable(test_fiber_mutex "tests/test_fiber_mutex.cc" "${LIBS}")
flexy_add_executable(test_this_fiber "tests/test_this_fiber.cc" "${LIBS}")
flexy_add_executable(test_fiber_condition_variable "tests/test_fiber_condition_variable.cc" "${LIBS}")

flexy_add_executable(test_function "tests/test_function.cc" "${TEST_LIBS}")

flexy_add_executable(echo_server "examples/echo_server.cc" "${LIBS}")
flexy_add_executable(chat_room "examples/chat_room.cc" "${LIBS}")
flexy_add_executable(http_server "examples/http_server.cc" "${LIBS}")
# flexy_add_executable(http2_server "examples/http2_server.cc" "${LIBS}")

flexy_add_executable(fiber "examples/fiber.cc" "${LIBS}")

endif(TESTS)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
