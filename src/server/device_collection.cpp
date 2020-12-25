//
// Created by yalavrinenko on 25.12.2020.
//
#include "../utils/logger.hpp"
#include <future>
#include <list>
#include <algorithm>

#include "device_collection.hpp"
void srp::device_collection::send_signal_implementation(srp::device_collection::signal_sender_callback const& signal_sender) {
  using futures = std::vector<std::pair<capture_device&, std::future<std::optional<ClientResponse>>>>;
  std::lock_guard lock(slave_lock_);

  futures runner; runner.reserve(slaves_.size());
  std::for_each(slaves_.begin(), slaves_.end(), [&runner, signal_sender](auto &slave){
    runner.emplace_back(signal_sender(slave));
  });

  auto monitor_function = [this](auto &runner) {
    auto result = runner.second.wait_for(DeviceCollectionOpt::wait_timeout());
    if (result == std::future_status::ready){
      process_result(runner.first, runner.second.get());
    } else {
      exclude_slave(runner.first);
    }
  };

  std::for_each(runner.begin(), runner.end(), monitor_function);
}
void srp::device_collection::exclude_slave(auto &slave) {
  slaves_.erase(std::find(slaves_.begin(), slaves_.end(), slave));
}
void srp::device_collection::on_start_callback(const std::string &path_template) {
  auto start_function = [this, &path_template](auto &slave) {
    return std::pair<capture_device &, std::future<std::optional<ClientResponse>>>{
        slave, std::async(std::launch::async, [this, &path_template, &slave]() { return slave->start_recording(path_template); })};
  };

  send_signal_implementation(start_function);
}
void srp::device_collection::on_stop_callback() {
  auto stop_function = [this](auto &slave) {
    return std::pair<capture_device &, std::future<std::optional<ClientResponse>>>{
        slave, std::async(std::launch::async, [this, &slave]() { return slave->stop_recording(); })};
  };

  send_signal_implementation(stop_function);
}
void srp::device_collection::on_sync_callback(size_t sync_point) {
  auto sync_time = [this, &sync_point](auto &slave) {
    return std::pair<capture_device &, std::future<std::optional<ClientResponse>>>{
        slave, std::async(std::launch::async, [this, &sync_point, &slave]() { return slave->sync_time(sync_point); })};
  };

  send_signal_implementation(sync_time);
}

void srp::device_collection::process_result(srp::capture_device &slave, const std::optional<ClientResponse> &response) {
  if (!response){
    LOGW << "Error in communication with " << slave->uid() << " Exclude from list. ";
    exclude_slave(slave);
  } else {
    LOGD << "OK";
  }
}
void srp::device_collection::add_capture_device(srp::capture_device device) {
  slaves_.push_back(std::move(device));
}
void srp::device_collection::remove_capture_device(size_t device_id) {
  exclude_slave(find_device(device_id));
}
srp::capture_device &srp::device_collection::find_device(size_t device_id) {
  auto device = std::find_if(slaves_.begin(), slaves_.end(), [device_id](auto const &dev){
    return dev->uid() == device_id;
  });
  if (device == slaves_.end())
    throw std::out_of_range("device id out of range");

  return *device;
}
