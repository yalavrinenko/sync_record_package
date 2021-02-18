cmake_minimum_required(VERSION 3.17)
project(SRP)

find_package(Boost COMPONENTS log log_setup REQUIRED)

set(BITALINO_SRC ${CMAKE_SOURCE_DIR}/3rd/bitalino/bitalino.cpp ${CMAKE_SOURCE_DIR}/3rd/bitalino/bitalino.h)
add_library(bitalino ${BITALINO_SRC})
if (NOT WIN32)
    target_compile_definitions(bitalino PUBLIC "-DHASBLUETOOTH")
endif()
if (NOT WIN32)
    target_link_libraries(bitalino bluetooth)
endif()

add_library(bitalino_instance bitalino_instance.hpp bitalino_instance.cpp bitalino_reader.cpp bitalino_reader.hpp)
target_link_libraries(bitalino_instance bitalino Boost::log Boost::log_setup)
target_include_directories(bitalino_instance PUBLIC "${CMAKE_SOURCE_DIR}/3rd/bitalino"
        "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}" "${Protobuf_INCLUDE_DIR}")

target_compile_definitions(bitalino_instance PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")