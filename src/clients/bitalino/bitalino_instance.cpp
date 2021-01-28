//
// Created by yalavrinenko on 26.01.2021.
//

#include "bitalino_instance.hpp"
#include "bitalino_reader.hpp"
#include <utils/logger.hpp>
#include <future>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <utils/io.hpp>

class srp::bitalino_instance::bitalino_io{
public:
  struct timestamp_entry {
    size_t batch_id;
    std::chrono::duration<double> batch_ts;
    double batch_p_sec;
  };

  using timestamp = srp::IoUtils::tsafe_timestamp<timestamp_entry>;

  struct io_block{
    std::ofstream stamps;
    std::ofstream data;

    std::unique_ptr<srp::bitalino_reader> device;

    std::atomic<bool> is_recording;
    timestamp last_ts;

    io_block(std::unique_ptr<srp::bitalino_reader> dev, std::filesystem::path const &data_path, std::filesystem::path const &split_path):
      device{std::move(dev)}, stamps{split_path}, data{data_path}{
      stamps << "#Frame\tBlockId\tPointId" << std::endl;
    }
  };

  struct recording_info{
    size_t capture_blocks;
    std::chrono::duration<double> capture_time;
    size_t frames;
  };

  explicit bitalino_io(srp::BitalinoCaptureOptions options): opt_{std::move(options)}{
  }

private:

  void store_frames(auto &io, auto block_id, auto frame_id, auto const &block_time, auto const &block_duration, auto const& frames){
    auto time_begin = (opt_.timestamp_mode() == srp::BitalinoCaptureOptions_TsMode_stepped) ? block_time - block_duration : block_time;

    auto step = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(block_duration) / frames.size());
    auto &oss = io->data;

    auto millis = [](auto const &duration) { return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); };

    for (BITalino::Frame const& frame: frames){
      oss << block_id << frame_id << " " << millis(time_begin) << " ";
      ++frame_id;
      if (opt_.timestamp_mode() == srp::BitalinoCaptureOptions_TsMode_stepped)
        time_begin += step;

      for (auto const &digit : frame.digital)
        oss << digit << " ";

      for (auto const &achannel : opt_.channels())
        oss << frame.analog[achannel] << " ";

      oss << "\n";
    }
  }

  recording_info capture_function(std::unique_ptr<io_block> &io){
    recording_info rinfo{.capture_blocks = 0,
    .capture_time = {},
    .frames = 0};
    LOGD << "Start recording on device at address: " << io->device->address();

    auto stime = std::chrono::high_resolution_clock::now();

    io->device->start();

    while (io->is_recording){
      auto acquire_start = std::chrono::high_resolution_clock::now();
      auto [ts, frames] = io->device->acquire();

      store_frames(io, rinfo.capture_blocks, rinfo.frames, ts - stime, ts - acquire_start, frames);

      timestamp_entry tentry{
          .batch_id = rinfo.capture_blocks,
          .batch_ts = ts - stime,
          .batch_p_sec = frames.size() / std::chrono::duration_cast<std::chrono::duration<double>>(ts - acquire_start).count()
      };

      ++rinfo.capture_blocks;
      rinfo.frames += frames.size();

      io->last_ts.update(tentry);
    }

    io->device->stop();

    auto ftime = std::chrono::high_resolution_clock::now();
    rinfo.capture_time = ftime - stime;

    LOGD << "Stop recording on device at address: " << io->device->address() << ". Recorded " << rinfo.capture_blocks << " blocks with "
        << io->device->block_size() << " at " << rinfo.capture_time.count() << " seconds.";
    return rinfo;
  }

  srp::BitalinoCaptureOptions opt_;
  std::unique_ptr<io_block> io_;
};

srp::bitalino_instance::bitalino_instance(srp::BitalinoCaptureOptions opt) {
  device_ = std::make_unique<bitalino_io>(std::move(opt));
}
void srp::bitalino_instance::init(size_t uid) {}

std::optional<srp::ClientCheckResponse> srp::bitalino_instance::check() { return std::optional<ClientCheckResponse>(); }
std::optional<srp::ClientStartRecordResponse> srp::bitalino_instance::start_recording(const std::string &path_template) {
  return std::optional<ClientStartRecordResponse>();
}
std::optional<srp::ClientStopRecordResponse> srp::bitalino_instance::stop_recording() { return std::optional<ClientStopRecordResponse>(); }
std::optional<srp::ClientSyncResponse> srp::bitalino_instance::sync_time(size_t sync_point) { return std::optional<ClientSyncResponse>(); }
std::optional<srp::ClientTimeResponse> srp::bitalino_instance::state() { return std::optional<ClientTimeResponse>(); }
srp::bitalino_instance::~bitalino_instance() = default;
