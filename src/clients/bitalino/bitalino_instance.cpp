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

  struct recording_info{
    size_t capture_blocks;
    std::chrono::duration<double> capture_time;
    size_t frames;
  };

  struct io_block{
    std::ofstream stamps;
    std::ofstream data;

    std::shared_ptr<srp::bitalino_reader> device;

    std::atomic<bool> is_recording;
    std::future<recording_info> record_thread;

    timestamp last_ts;

    io_block(std::shared_ptr<srp::bitalino_reader> dev, std::filesystem::path const &data_path, std::filesystem::path const &split_path):
      stamps{split_path}, data{data_path}, device{std::move(dev)}{
    }
  };

  explicit bitalino_io(srp::BitalinoCaptureOptions options): opt_{std::move(options)}{
  }

  auto check_device();

  auto start_recording(std::filesystem::path const &path_template, std::string const& dev_id);
  auto stop_recording();
  auto send_sync_stamp(size_t id);
  std::optional<timestamp_entry> recording_state() const;

  auto name() const {
    return opt_.device();
  }

private:
  template<typename ... dev_init_t>
  static auto aquire_device(dev_init_t ... args){
    static std::shared_ptr<srp::bitalino_reader> reader{ nullptr};
    if (reader == nullptr)
      reader = std::make_shared<srp::bitalino_reader>(args...);

    return reader;
  }

  auto init_io_block(std::string const& data_path, std::string const& split_path) {
    std::vector<int> channels;
    for (auto c : opt_.channels())
      channels.emplace_back(c - 1);

    auto dev = aquire_device(opt_.device(), opt_.sampling_rate(), opt_.block_size(), channels);
    auto io = std::make_unique<io_block>(std::move(dev), data_path, split_path);

    io->stamps << "#BlockId\tTimestamp[millis]\tPointId" << std::endl;
    io->data << "#BlockID\tFrameID\tTimestamp[millis]\tD0\tD1\tD2\tD3\t";
    for (auto c : channels)
      io->data << "A" + std::to_string(c + 1) << "\t";
    io->data << "\n";

    return io;
  }

  void store_frames(auto &io, auto block_id, auto frame_id, auto const &block_time, auto const &block_duration, auto const& frames){
    auto time_begin = (opt_.timestamp_mode() == srp::BitalinoCaptureOptions_TsMode_stepped) ? block_time - block_duration : block_time;

    auto step = std::chrono::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(block_duration) / frames.size());
    auto &oss = io->data;

    auto millis = [](auto const &duration) { return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(); };

    for (BITalino::Frame const& frame: frames){
      oss << block_id << "\t" << frame_id << "\t" << millis(time_begin) << "\t";
      ++frame_id;
      if (opt_.timestamp_mode() == srp::BitalinoCaptureOptions_TsMode_stepped)
        time_begin += step;

      for (auto const &digit : frame.digital)
        oss << digit << "\t";

      for (auto const &achannel : opt_.channels())
        oss << frame.analog[achannel] << "\t";

      oss << "\n";
    }
  }

  recording_info capture_function(std::unique_ptr<io_block> &io){
    recording_info rinfo{.capture_blocks = 0,
    .capture_time = {},
    .frames = 0};
    LOGD << "Start recording on device at address: " << io->device->address();

    io->device->start();

    auto stime = std::chrono::high_resolution_clock::now();
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
    auto ftime = std::chrono::high_resolution_clock::now();

    io->device->stop();
    rinfo.capture_time = ftime - stime;

    LOGD << "Stop recording on device at address: " << io->device->address() << ". Recorded " << rinfo.capture_blocks << " blocks with size "
        << io->device->block_size() << " at " << rinfo.capture_time.count() << " seconds.";
    return rinfo;
  }

  srp::BitalinoCaptureOptions opt_;
  std::unique_ptr<io_block> io_;
};
auto srp::bitalino_instance::bitalino_io::check_device() {
  try {
    auto check_output = "check." + opt_.filetype();
    auto stamp_output = "check.stamp";

    auto io = init_io_block(check_output, stamp_output);

    io->record_thread = std::async(std::launch::async, [this, &io]() {
      io->is_recording = true;
      return capture_function(io);
    });

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    io->is_recording = false;

    auto check_info = io->record_thread.get();

    std::ostringstream oss;
    oss << "Perform check for Bitalino device " << io->device->address() << " version " << io->device->device_info() <<".\n";
    oss << "Capture " << check_info.frames << " frames [" << std::chrono::duration_cast<std::chrono::milliseconds>(check_info.capture_time).count()
        << " millis] at " << opt_.sampling_rate() << " smpl/s\n";
    oss << "Stored to " << check_output << " and " << stamp_output << std::endl;
    io.reset(nullptr);

    LOGD << oss.str();
    return std::pair{true, oss.str()};
  } catch (std::exception &e) {
    return std::pair{false, "Catch critical exception. Message: " + std::string(e.what())};
  } catch (BITalino::Exception &e){
    return std::pair{false, "Bitalino raise exception[" + std::to_string(e.code) + "]. Message: " + std::string(e.getDescription())};
  }
}
auto srp::bitalino_instance::bitalino_io::start_recording(const std::filesystem::path &path_template, std::string const& dev_id) {
  auto [output_path, stamp_path] = srp::PathUtils::create_file_path(opt_.root(), path_template, dev_id, opt_.filetype());

  io_ = init_io_block(output_path, stamp_path);

  io_->record_thread = std::async(std::launch::async, [this]() {
    io_->is_recording = true;
    return capture_function(io_);
  });

  return std::pair{output_path, stamp_path};
}
auto srp::bitalino_instance::bitalino_io::stop_recording() {
  if (io_) {
    io_->is_recording = false;
    try {
      auto rec_result = io_->record_thread.get();

      io_.reset(nullptr);

      return std::pair{rec_result.frames, rec_result.capture_time};
    } catch (std::exception &e) {
      LOGE << "Error during recordion. Reason: " << e.what();

      io_.reset(nullptr);
      return std::pair{static_cast<size_t>(0), std::chrono::duration<double>{}};
    }
  } else {
    return std::pair{static_cast<size_t>(1), std::chrono::duration<double>{}};
  }
}
auto srp::bitalino_instance::bitalino_io::send_sync_stamp(size_t sync_id) {
  if (io_) {
    auto current_frame = io_->last_ts.ts();
    io_->stamps << current_frame.batch_id << "\t" << std::chrono::duration_cast<std::chrono::milliseconds>(current_frame.batch_ts).count()
                << "\t" << sync_id << std::endl;

    return std::tuple{current_frame.batch_id, current_frame.batch_ts, current_frame.batch_p_sec};
  } else {
    return std::tuple{static_cast<size_t>(0ul), std::chrono::duration<double>{}, -1.0};
  }
}
std::optional<srp::bitalino_instance::bitalino_io::timestamp_entry> srp::bitalino_instance::bitalino_io::recording_state() const {
  if (io_) {
    return io_->last_ts.ts();
  } else {
    return {};
  }
}

srp::bitalino_instance::bitalino_instance(srp::BitalinoCaptureOptions opt) {
  device_ = std::make_unique<bitalino_io>(std::move(opt));
}
void srp::bitalino_instance::init(size_t uid) { this->uid_ = uid; }

std::optional<srp::ClientCheckResponse> srp::bitalino_instance::check() {
  auto [is_ok, ack] = device_->check_device();
  if (!is_ok)
    LOGW << ack;

  ClientCheckResponse resp;
  resp.set_check_ok(is_ok);
  resp.set_info(ack);
  resp.set_type("Bitalino");
  resp.set_name(device_->name());
  return resp;
}


std::optional<srp::ClientStartRecordResponse> srp::bitalino_instance::start_recording(const std::string &path_template) {
  ClientStartRecordResponse r;
  auto [output, stamp] = device_->start_recording(path_template, std::to_string(uid()));
  r.add_data_path(output);
  r.add_sync_point_path(output);

  return r;
}
std::optional<srp::ClientStopRecordResponse> srp::bitalino_instance::stop_recording() {
  srp::ClientStopRecordResponse r;

  auto [frames, duration] = device_->stop_recording();

  if (frames == 0 && duration == std::chrono::duration<double>{}) return {};

  r.set_frames(frames);
  r.set_duration_sec(duration.count());
  r.set_average_fps(frames / duration.count());

  return r;
}

std::optional<srp::ClientSyncResponse> srp::bitalino_instance::sync_time(size_t sync_point) {
  srp::ClientSyncResponse r;

  auto [frame, duration, fps] = device_->send_sync_stamp(sync_point);

  if (fps < 0) { return {}; }

  r.set_sync_point(sync_point);
  r.set_average_fps(fps);
  r.set_frames(frame);
  r.set_duration_sec(duration.count());

  return r;
}
std::optional<srp::ClientTimeResponse> srp::bitalino_instance::state() {
  ClientTimeResponse ctr;
  auto state = device_->recording_state();
  if (state) {
    ctr.set_state(ClientTimeResponse_CaptureState_recording);
    ctr.set_frames(state->batch_id);
    ctr.set_average_fps(state->batch_p_sec);
    ctr.set_duration_sec(state->batch_ts.count());
  } else {
    ctr.set_state(ClientTimeResponse_CaptureState_stopped);
  }
  return ctr;
}
srp::bitalino_instance::~bitalino_instance() = default;
