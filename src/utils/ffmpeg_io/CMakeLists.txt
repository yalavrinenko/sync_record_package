cmake_minimum_required(VERSION 3.17)
project(SRP)

find_package(ffmpeg REQUIRED)
find_package(Boost COMPONENTS log log_setup REQUIRED)

add_library(ffmpeg_io ffmpeg_reader.cpp exceptions.hpp ffmpeg_io.hpp ffmpeg_io.cpp ffmpeg_writer.cpp ffmpeg_writer.hpp notified_dequeu.hpp)
target_include_directories(ffmpeg_io PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(ffmpeg_io ${FFMPEG_LIBRARIES} Boost::log Boost::log_setup)
target_compile_definitions(ffmpeg_io PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")