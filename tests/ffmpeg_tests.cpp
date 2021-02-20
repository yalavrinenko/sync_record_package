//
// Created by yalavrinenko on 20.02.2021.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <utils/ffmpeg_io/ffmpeg_reader.hpp>
#include <utils/ffmpeg_io/ffmpeg_writer.hpp>

#include <utils/ffmpeg_io/ffmpeg_reader.cpp>
#include <utils/ffmpeg_io/ffmpeg_writer.cpp>

BOOST_AUTO_TEST_SUITE(FFmpegReader);

BOOST_AUTO_TEST_CASE(Open_and_close) {
  srp::ffmpeg_io_container::io_device device{
      .name = "plughw:0,0",
      .format = "alsa",
  };
  for (auto i = 0; i < 100; ++i) { BOOST_REQUIRE_NO_THROW(srp::ffmpeg_reader reader(device)); }
}

namespace {
  unsigned long frame_index = 0;
}

BOOST_AUTO_TEST_CASE(Read_dummy_frames) {
  srp::ffmpeg_io_container::io_device device{
      .name = "plughw:0,0",
      .format = "alsa",
  };
  std::unique_ptr<srp::ffmpeg_reader> in;
  BOOST_REQUIRE_NO_THROW(in = std::make_unique<srp::ffmpeg_reader>(device));

  in->select_stream(0);

  struct dummy_frame{
    dummy_frame(AVFrame* frame){
      ++frame_index;
    }
  };

  for (auto i = 0; i < 10; ++i)
    in->read<dummy_frame>();

  LOGD << frame_index;
}

BOOST_AUTO_TEST_CASE(Read_native_frame) {
  srp::ffmpeg_io_container::io_device device{
      .name = "plughw:0,0",
      .format = "alsa",
  };
  srp::ffmpeg_reader in(device);

  in.select_stream(0);

  struct dummy_frame{
    dummy_frame(AVFrame* frame){
      frame_ = av_frame_clone(frame);
    }
    ~dummy_frame(){
      av_frame_free(&frame_);
    }
    AVFrame *frame_;
  };

  for (auto i = 0; i < 10; ++i) {
    srp::native_audio_frame f;
    in >> f;
    LOGD << f.frame->pts << " " << f.frame->pkt_dts << " " << f.frame->pkt_duration << " " << f.frame->linesize[0];
  }

  LOGD << frame_index;
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(FFmpegWriter);
BOOST_AUTO_TEST_CASE(OpenCloseReadWrite){
    srp::ffmpeg_io_container::io_device device{
        .name = "plughw:0,0",
        .format = "alsa",
    };

    srp::ffmpeg_reader input(device);
    input.select_stream(0);

    srp::native_audio_frame frame1, frame2;
    input >> frame1 >> frame2;
    auto raw_frame_dpts = frame1.pts_delta(frame2);

  srp::ffmpeg_writer fw("../tests/output/test.wav");
  srp::ffmpeg_io_container::ffmpeg_stream::stream_options opt{
      .type = srp::ffmpeg_io_container::ffmpeg_stream::stream_options::stream_type::audio,
      .bitrate = 192000,
      .pts_step = static_cast<size_t>(raw_frame_dpts),
      .audio_opt = {
          .sample_rate = 44100
      }
  };

  fw.create_stream(opt);

  for (auto i = 0; i < 100; ++i){
    srp::native_audio_frame frame;
    input >> frame;
    fw << frame;
  }
}
BOOST_AUTO_TEST_SUITE_END();