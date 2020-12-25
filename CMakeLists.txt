cmake_minimum_required(VERSION 3.15)
project(SRP)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

set(SERVER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src")
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
        src/server/sessions.hpp src/server/server_acceptor.cpp
        src/server/rec_client.cpp src/server/rec_client.hpp
        src/server/device_collection.cpp src/server/device_collection.hpp
        src/capture_interface/capture_control.hpp)

set(SERVER_TARGET_LIB srp_server)

add_library(${SERVER_TARGET_LIB}  ${SERVER_SRC})
target_link_libraries(${SERVER_TARGET_LIB} ${SERVER_LIBS})
target_include_directories(${SERVER_TARGET_LIB} PUBLIC ${SERVER_INCLUDES})
target_compile_definitions(${SERVER_TARGET_LIB} PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

set(SERVER_TARGET sync_server)
add_executable(${SERVER_TARGET}  src/server/server.cpp)
target_link_libraries(${SERVER_TARGET} ${SERVER_TARGET_LIB})
target_include_directories(${SERVER_TARGET} PUBLIC ${SERVER_INCLUDES})

function(create_test_set test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} ${SERVER_TARGET_LIB} Boost::unit_test_framework)

    add_executable(${test_name}_address ${source_file})
    target_link_libraries(${test_name}_address ${SERVER_TARGET_LIB} Boost::unit_test_framework)
    target_compile_options(${test_name}_address PRIVATE -fsanitize=address)
    target_link_options(${test_name}_address PRIVATE -fsanitize=address)

    add_executable(${test_name}_threads ${source_file})
    target_link_libraries(${test_name}_threads ${SERVER_TARGET_LIB} Boost::unit_test_framework)
    target_compile_options(${test_name}_threads PRIVATE -fsanitize=thread)
    target_link_options(${test_name}_threads PRIVATE -fsanitize=thread)

    add_executable(${test_name}_ub ${source_file})
    target_link_libraries(${test_name}_ub ${SERVER_TARGET_LIB} Boost::unit_test_framework)
    target_compile_options(${test_name}_ub PRIVATE -fsanitize=undefined)
    target_link_options(${test_name}_ub PRIVATE -fsanitize=undefined)
endfunction()

create_test_set(server_test_entry tests/server_test.cpp)
create_test_set(client_test_entry tests/client_test.cpp)
create_test_set(device_collection_test_entry  tests/device_collection_test.cpp)