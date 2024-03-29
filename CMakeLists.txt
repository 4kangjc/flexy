cmake_minimum_required(VERSION 3.2)
project(flexy CXX C ASM)

include(cmake/utils.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(./)

file(GLOB fcontext "flexy/fiber/internal/x86_64/*.S")
file(GLOB fiber "flexy/fiber/*.cpp")
file(GLOB thread "flexy/thread/*.cpp")
file(GLOB schedule "flexy/schedule/*.cpp")
file(GLOB net "flexy/net/*.cpp")
file(GLOB http "flexy/http/*.cpp")
file(GLOB db "flexy/db/*.cpp")
file(GLOB stream "flexy/stream/*.cpp")
file(GLOB http2 "flexy/http2/*.cpp")
file(GLOB util "flexy/util/*.cpp")
file(GLOB env "flexy/env/*.cpp")

# set(LIB_SRC
#     ${fcontext}
#     ${fiber}
#     ${thread}
#     ${schedule}
#     ${net}
#     ${http}
#     ${db}
#     ${stream}
#     ${http2}
#     ${util}
#     ${env}
# )

set(LIB_SRC
    flexy/thread/thread.cpp
    flexy/util/util.cpp
    flexy/util/log.cpp
    flexy/util/file.cpp
    flexy/fiber/fiber.cpp
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
    flexy/fiber/mutex.cpp 
    flexy/net/bytearray.cpp
    flexy/http2/huffman.cpp
    flexy/http2/frame.cpp
    flexy/http2/hpack.cpp
    # flexy/http2/stream.cpp 
    # flexy/http2/http2_stream.cpp 
    # flexy/http2/http2_server.cpp
    flexy/fiber/condition_variable.cpp
)

list(APPEND LIB_SRC ${fcontext})

ragelmaker(flexy/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/flexy/http)
ragelmaker(flexy/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/flexy/http)

set(LIBS
    flexy
    pthread
    fmt
    yaml-cpp
    jsoncpp
    dl
    mysqlclient
    sqlite3
    ssl
    crypto
)

option(FLEXY_YAML "Config by Yaml" ON)
option(FLEXY_JSON "Config by Json" OFF)
option(BUILD_SHARED_LIBS "Build flexy as a shared lib" ON)
option(BUILD_STATIC_LIBS "Build flext as a static lib" OFF)

if(FLEXY_JSON)
    add_definitions(-DFLEXY_JSON)
elseif(FLEXY_YAML)
    add_definitions(-DFLEXY_YAML)
endif(FLEXY_JSON)

if (BUILD_STATIC_LIBS)
add_library(flexy STATIC ${LIB_SRC})
target_link_libraries(${LIBS})
elseif (BUILD_SHARED_LIBS)
add_library(flexy SHARED ${LIB_SRC})
target_link_libraries(${LIBS})
endif(BUILD_STATIC_LIBS)

redefine_file_macro(flexy)

option(FLEXY_TESTS "flexy tests execute" ON)

if(FLEXY_TESTS)

set (GTEST_LIBS ${LIBS} gtest)
enable_testing()

add_subdirectory(tests)

endif(FLEXY_TESTS)

flexy_add_executable(echo_server "examples/echo_server.cc" "${LIBS}")
flexy_add_executable(chat_room "examples/chat_room.cc" "${LIBS}")
flexy_add_executable(http_server "examples/http_server.cc" "${LIBS}")
# flexy_add_executable(http2_server "examples/http2_server.cc" "${LIBS}")
flexy_add_executable(fiber "examples/fiber.cc" "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/lib)
