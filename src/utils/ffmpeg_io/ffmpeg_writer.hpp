//
// Created by yalavrinenko on 15.01.2021.
//

#ifndef SRP_FFMPEG_WRITER_HPP
#define SRP_FFMPEG_WRITER_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include "ffmpeg_io.hpp"
#include "notified_dequeu.hpp"
#include <future>

struct AVFrame;

namespace srp {
  class ffmpeg_writer {
  public:
    explicit ffmpeg_writer(std::string const &source);

    void create_stream(ffmpeg_io_container::ffmpeg_stream::stream_options const& options);

    template<typename frame_t>
    ffmpeg_writer& write_frame(frame_t const &frame){
      auto av_frame = frame.to_avframe(frame_pts_);
      packed_frame pf{
          .frame = av_frame,
          .pts = frame_pts_
      };
      frame_pts_ += stream_->option().pts_step;

      dequeue_frame(pf);

      return *this;
    }

    ~ffmpeg_writer();

  protected:
    struct packed_frame{
      AVFrame* frame;
      size_t pts;

      void release();
    };

    void dequeue_frame(packed_frame const& pframe);

    size_t frame_pts_{0};
    ffmpeg_io_container container_;
    std::unique_ptr<ffmpeg_io_container::ffmpeg_stream> stream_ = nullptr;
    notified_deque<packed_frame> frame_queue_;
    std::atomic<bool> be_active_;
    std::future<size_t> writer_thread_;
  };

  template <typename frame_t>
  ffmpeg_writer& operator << (ffmpeg_writer &out, frame_t const& frame){
    return out.template write_frame(frame);
  }

}// namespace srp



#endif//SRP_FFMPEG_WRITER_HPP
