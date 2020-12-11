cmake_minimum_required(VERSION 3.15)
project(SRP)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

set(SERVER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SERVER_LIBS)

set( Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost" )
find_package(Boost COMPONENTS log log_setup system unit_test_framework REQUIRED)

find_package(Protobuf REQUIRED)
set(Protobuf_IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/protocols")

set(SERVER_INCLUDES ${TARGET_INCLUDES} Boost::headers ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set(SERVER_LIBS ${SERVER_LIBS} Boost::log Boost::log_setup Boost::thread Boost::system protobuf::libprotobuf)

protobuf_generate_cpp(SESSION_PROTO_SRCS SESSION_PROTO_HDRS ./src/protocols/session.proto)

set(SERVER_SRC ${SESSION_PROTO_SRCS}
        src/server/control_server.cpp
        src/server/sessions.cpp
        src/server/sessions.hpp src/server/server_acceptor.cpp)

set(SERVER_TARGET_LIB srp_server)

add_library(${SERVER_TARGET_LIB}  ${SERVER_SRC})
target_link_libraries(${SERVER_TARGET_LIB} ${SERVER_LIBS})
target_include_directories(${SERVER_TARGET_LIB} PUBLIC ${SERVER_INCLUDES})
target_compile_definitions(${SERVER_TARGET_LIB} PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")


set(SERVER_TARGET sync_server)
add_executable(${SERVER_TARGET}  src/server/server.cpp)
target_link_libraries(${SERVER_TARGET} ${SERVER_TARGET_LIB})
target_include_directories(${SERVER_TARGET} PUBLIC ${SERVER_INCLUDES})

set(SERVER_TESTS_SRC tests/server_test.cpp)

add_executable(server_test_entry_address ${SERVER_TESTS_SRC})
target_link_libraries(server_test_entry_address ${SERVER_TARGET_LIB} Boost::unit_test_framework)
target_compile_options(server_test_entry_address PRIVATE -fsanitize=address)
target_link_options(server_test_entry_address PRIVATE -fsanitize=address)

add_executable(server_test_entry_threads ${SERVER_TESTS_SRC})
target_link_libraries(server_test_entry_threads ${SERVER_TARGET_LIB} Boost::unit_test_framework)
target_compile_options(server_test_entry_threads PRIVATE -fsanitize=thread)
target_link_options(server_test_entry_threads PRIVATE -fsanitize=thread)

add_executable(server_test_entry_ub ${SERVER_TESTS_SRC})
target_link_libraries(server_test_entry_ub ${SERVER_TARGET_LIB} Boost::unit_test_framework)
target_compile_options(server_test_entry_ub PRIVATE -fsanitize=undefined)
target_link_options(server_test_entry_ub PRIVATE -fsanitize=undefined)