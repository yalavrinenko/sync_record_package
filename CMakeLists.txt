cmake_minimum_required(VERSION 3.15)
project(SRP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror")

set(SERVER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SERVER_LIBS)

set( Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost" )
find_package(Boost COMPONENTS log log_setup system REQUIRED)

set(SERVER_INCLUDES ${TARGET_INCLUDES} Boost::headers)
set(SERVER_LIBS ${SERVER_LIBS} Boost::log Boost::log_setup Boost::thread Boost::system)

set(SERVER_SRC src/server/server.cpp src/server/control_server.cpp src/server/sessions.cpp src/server/sessions.hpp)
set(SERVER_TARGET sync_server)

add_executable(${SERVER_TARGET} ${SERVER_SRC})
target_link_libraries(${SERVER_TARGET} ${SERVER_LIBS})
target_include_directories(${SERVER_TARGET} PUBLIC ${SERVER_INCLUDES})
target_compile_definitions(${SERVER_TARGET} PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")
