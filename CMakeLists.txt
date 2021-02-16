cmake_minimum_required(VERSION 3.17)
project(SRP)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

add_subdirectory(src/utils/ffmpeg_io)

set(SERVER_LIBS)

set( Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost" )
find_package(Boost COMPONENTS log log_setup system unit_test_framework REQUIRED)

find_package(Protobuf REQUIRED)
set(Protobuf_IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/protocols")

set(SERVER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src ${TARGET_INCLUDES} Boost::headers ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(SERVER_LIBS ${SERVER_LIBS} Boost::log Boost::log_setup Boost::thread Boost::system protobuf::libprotobuf)

if (WIN32)
    set(SERVER_LIBS ${SERVER_LIBS} -lws2_32 -lwsock32)
endif()

protobuf_generate_cpp(SESSION_PROTO_SRCS SESSION_PROTO_HDRS ./src/protocols/session.proto)
protobuf_generate_cpp(CLIENT_OPTION_PROTO_SRCS CLIENT_OPTION_PROTO_HDRS ./src/protocols/options.proto)

set(SERVER_SRC ${SESSION_PROTO_SRCS} ${CLIENT_OPTION_PROTO_SRCS}
        src/server/control_server.cpp
        src/net/sessions.cpp
        src/net/sessions.hpp src/server/server_acceptor.cpp
        src/server/rec_client.cpp src/server/rec_client.hpp
        src/server/device_collection.cpp src/server/device_collection.hpp
        src/capture_interface/capture_control.hpp
        src/net/netcomm.cpp src/server/controlled_collection.cpp
        src/server/controlled_collection.hpp src/server/remote_control_client.cpp
        src/server/remote_control_client.hpp)

set(SERVER_TARGET_LIB server_core)

add_library(${SERVER_TARGET_LIB}  ${SERVER_SRC})
target_link_libraries(${SERVER_TARGET_LIB} ${SERVER_LIBS})
target_include_directories(${SERVER_TARGET_LIB} PUBLIC ${SERVER_INCLUDES})
target_compile_definitions(${SERVER_TARGET_LIB} PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

set(SERVER_TARGET sync_server)
add_executable(${SERVER_TARGET}  src/server/server.cpp)
target_link_libraries(${SERVER_TARGET} ${SERVER_TARGET_LIB})
target_include_directories(${SERVER_TARGET} PUBLIC ${SERVER_INCLUDES})

set(CLIENT_COMMON_LIB netclient)
set(CLIENT_COMMON_SRC ${SESSION_PROTO_SRCS} ${CLIENT_OPTION_PROTO_SRCS}
        src/net/sessions.cpp src/net/netcomm.cpp
        src/clients/netclient.cpp src/clients/netclient.hpp)

set(CLIENT_INCLUDES ${TARGET_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/src Boost::headers ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set(CLIENT_LIBS ${SERVER_LIBS} Boost::log Boost::log_setup Boost::thread Boost::system protobuf::libprotobuf)

add_library(${CLIENT_COMMON_LIB} ${CLIENT_COMMON_SRC})
target_link_libraries(${CLIENT_COMMON_LIB} ${CLIENT_LIBS})
target_include_directories(${CLIENT_COMMON_LIB} PUBLIC ${CLIENT_INCLUDES})
target_compile_definitions(${CLIENT_COMMON_LIB} PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

add_subdirectory(src/clients/control)

add_subdirectory(src/clients/audio)

add_subdirectory(src/clients/bitalino)

add_executable(capture_client ${CLIENT_OPTION_PROTO_SRCS} src/clients/capture_client.cpp)
target_link_libraries(capture_client ffmpeg_io
        audio_instance bitalino_instance
        ${CLIENT_COMMON_LIB})
target_compile_definitions(capture_client PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

add_executable(control_client ${CLIENT_OPTION_PROTO_SRCS} src/clients/control.cpp)
target_link_libraries(control_client ui_control ${CLIENT_COMMON_LIB} Boost::log Boost::log_setup Boost::thread Boost::system)
target_compile_definitions(control_client PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")


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


add_executable(streams 3rd/stream_tests.cpp ${SESSION_PROTO_SRCS})
target_link_libraries(streams ${SERVER_LIBS})
target_include_directories(streams PUBLIC ${SERVER_INCLUDES})
target_compile_definitions(streams PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

add_executable(capture_client_test tests/audio_client_test.cpp)
target_link_libraries(capture_client_test ffmpeg_io ${CLIENT_COMMON_LIB}
        audio_instance)