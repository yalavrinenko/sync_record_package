//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_FFMPEG_READER_HPP
#define SRP_FFMPEG_READER_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include "ffmpeg_io.hpp"

struct AVFrame;

namespace srp {
  class ffmpeg_reader {
  public:
    explicit ffmpeg_reader(std::string const &source);

    void select_stream(unsigned stream_id);

    template<typename frame_t>
    std::optional<frame_t> read() {
      if (stream_ == nullptr)
        throw std::logic_error("Stream not open!");

      auto eof = stream_->extract_frame(local_frame_);
      if (!eof) {
        eof_ = false;
        return frame_t(local_frame_);
      }
      else {
        eof_ = true;
        return {};
      }
    }

    explicit operator bool() const {
      return !eof_;
    }

  protected:
    bool eof_ = false;
    AVFrame *local_frame_;
    ffmpeg_io_container container_;
    std::unique_ptr<ffmpeg_io_container::ffmpeg_stream> stream_ = nullptr;
  };

  template<typename frame_t>
  ffmpeg_reader& operator >> (ffmpeg_reader& in, frame_t &frame){
    auto frame_opt = in.template read<frame_t>();
    if (frame_opt)
      frame = std::move(*frame_opt);

    return in;
  }

}// namespace srp


#endif//SRP_FFMPEG_READER_HPP
