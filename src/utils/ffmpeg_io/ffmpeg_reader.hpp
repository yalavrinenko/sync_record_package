//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_FFMPEG_READER_HPP
#define SRP_FFMPEG_READER_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

struct AVFormatContext;
struct AVCodecParameters;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace srp {
  struct audio_frame {
    audio_frame() = default;
    explicit audio_frame(AVFrame* raw_frame);

    using samples = std::vector<uint8_t>;
    std::vector<samples> data{};
    int nb_samples{};
    int format{};
    int sample_rate{};
    int64_t pts{};
  };

  class video_frame {};

  class ffmpeg_io_container {
  public:
    class ffmpeg_stream {
    public:
      ffmpeg_stream() = default;
      ffmpeg_stream(AVFormatContext *linked_context, AVStream *linked_stream);

      bool extract_frame(AVFrame *frame);

      ~ffmpeg_stream();

    protected:
      AVFormatContext *linked_context_{};
      AVCodecParameters *codec_par_ = nullptr;
      AVStream *stream_ = nullptr;
      AVCodec *codec_ = nullptr;
      AVCodecContext *coder_context_ = nullptr;
      AVPacket *packet_ = nullptr;
    };

    explicit ffmpeg_io_container(std::string const &source);

    [[nodiscard]] unsigned streams_count() const;

    std::unique_ptr<ffmpeg_stream> open_stream(unsigned int stream_index);

    ~ffmpeg_io_container();

  private:
    AVFormatContext *context_ptr_ = nullptr;
  };

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
