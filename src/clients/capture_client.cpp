//
// Created by yalavrinenko on 12.01.2021.
//

#include "netclient.hpp"
#include <utils/ffmpeg_io/ffmpeg_reader.hpp>
#include <utils/ffmpeg_io/ffmpeg_writer.hpp>
#include <utils/ffmpeg_io/notified_dequeu.hpp>

#include <future>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}


int main(int argc, char** argv){
  srp::ffmpeg_reader reader({"plughw:0,0", "alsa"});
  reader.select_stream(0);

  srp::ffmpeg_writer writer("output.wav");

  srp::ffmpeg_io_container::ffmpeg_stream::stream_options options{
      .type = srp::ffmpeg_io_container::ffmpeg_stream::stream_options::stream_type::audio,
      .bitrate = static_cast<size_t>(reader.stream()->context()->bit_rate),
      .pts_step = 1024,
      .audio_opt = {.sample_rate = 44100}
  };
  writer.create_stream(options);


  srp::native_audio_frame frame;

  while (reader >> frame){
    if (frame.frame) {
//      std::cout << frame.frame->pts / 44100.0 << " " << frame.frame->pts << " " << frame.frame->sample_rate << " " << frame.frame->nb_samples << " "
//                << frame.frame->linesize[0] << std::endl;
      writer << frame;
    }
  }

  return 0;
}