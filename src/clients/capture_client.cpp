//
// Created by yalavrinenko on 12.01.2021.
//

#include "netclient.hpp"
#include <utils/ffmpeg_io/ffmpeg_reader.hpp>

int main(int argc, char** argv){

  srp::ffmpeg_reader reader("file:///home/yalavrinenko/Files/git/sync_record_package/tests/video.mp4");

  reader.select_stream(1);

  srp::audio_frame frame;

  while (reader >> frame && !frame.data.empty()){
    if (!frame.data.empty())
      std::cout << frame.pts << " " << frame.data.front().size() << std::endl;
  }

  return 0;
}