//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_FFMPEG_READER_HPP
#define SRP_FFMPEG_READER_HPP

#include <filesystem>
#include <string>

struct AVFormatContext;
struct AVCodecParameters;
struct AVStream;
struct AVCodec;
struct AVCodecContext;

namespace srp {
  class ffmpeg_io_container{
  public:

    class ffmpeg_stream{
    public:
      ffmpeg_stream(AVFormatContext const* linked_context, AVStream* linked_stream);

      ~ffmpeg_stream();
    protected:
      AVFormatContext const* linked_context_;
      AVCodecParameters *codec_par_ = nullptr;
      AVStream* stream_ = nullptr;
      AVCodec* codec_ = nullptr;
      AVCodecContext* coder_context_ = nullptr;
    };

    explicit ffmpeg_io_container(std::string const &source);

    unsigned streams_count() const;

    std::unique_ptr<ffmpeg_stream> open_stream(unsigned int stream_index);

    ~ffmpeg_io_container();
  private:
    AVFormatContext* context_ptr_ = nullptr;
  };

  class ffmpeg_reader {
  public:
    //explicit ffmpeg_reader(std::filesystem::path const &file_path);

    explicit ffmpeg_reader(std::string const &source);

  protected:
    ffmpeg_io_container container_;
  };

}


#endif//SRP_FFMPEG_READER_HPP
