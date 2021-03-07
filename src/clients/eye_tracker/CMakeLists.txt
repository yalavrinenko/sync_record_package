cmake_minimum_required(VERSION 3.17)
project(SRP)

find_package(Boost COMPONENTS log log_setup REQUIRED)
find_package(cppzmq REQUIRED)

set(PUPIL_SRC pupil_eye_io.cpp pupil_eye_io.hpp pupil_eyetracker_instance.cpp pupil_eyetracker_instance.hpp)

add_library(pupil_io_instance ${PUPIL_SRC})
target_link_libraries(pupil_io_instance bitalino Boost::log Boost::log_setup cppzmq)
target_include_directories(pupil_io_instance PUBLIC "${CMAKE_SOURCE_DIR}/3rd/bitalino"
        "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}" "${Protobuf_INCLUDE_DIR}")

target_compile_definitions(pupil_io_instance PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")