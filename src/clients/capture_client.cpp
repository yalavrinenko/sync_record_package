//
// Created by yalavrinenko on 12.01.2021.
//

#include "netclient.hpp"
#include <utils/ffmpeg_io/ffmpeg_reader.hpp>

int main(int argc, char** argv){

  srp::ffmpeg_reader reader("file:///home/yalavrinenko/Files/git/sync_record_package/tests/video.mp4");

  return 0;
}