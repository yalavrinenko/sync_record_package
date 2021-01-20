//
// Created by yalavrinenko on 25.12.2020.
//
#include "../utils/logger.hpp"
#include <algorithm>
#include <future>
#include <list>
#include <protocols/actions.hpp>
#include <utils/io.hpp>

#include "device_collection.hpp"

void srp::device_collection::signal_to_device(srp::capture_device &device, const srp::device_collection::signal_sender_callback &signal_sender) {
  auto result = signal_sender(device);
  process_result(result.first, result.second.get());
}

void srp::device_collection::signal_broadcast(srp::device_collection::signal_sender_callback const &signal_sender) {
  using futures = std::vector<std::pair<capture_device &, std::future<std::optional<ClientResponse>>>>;
  std::lock_guard lock(slave_lock_);

  futures runner;
  runner.reserve(slaves_.size());
  std::for_each(slaves_.begin(), slaves_.end(), [&runner, signal_sender](auto &slave) { runner.emplace_back(signal_sender(slave)); });

  auto monitor_function = [this](auto &runner) {
    auto result = runner.second.wait_for(DeviceCollectionOpt::wait_timeout());
    if (result == std::future_status::ready) {
      process_result(runner.first, runner.second.get());
    } else {
      exclude_slave(runner.first);
    }
  };

  std::for_each(runner.begin(), runner.end(), monitor_function);
}
void srp::device_collection::exclude_slave(auto &slave) { slaves_.erase(std::find(slaves_.begin(), slaves_.end(), slave)); }

template<typename proto_response_t>
std::optional<srp::ClientResponse> puck_income_message(std::optional<proto_response_t> const &resp, size_t uid, srp::ActionType trigger) {
  if (resp) {
    srp::ClientResponse crp;
    crp.set_trigger_action(trigger);
    crp.set_uid(uid);

    std::string data;
    resp->SerializeToString(&data);
    crp.set_data(data);
    return crp;
  } else {
    return {};
  }
}

using async_response = std::future<std::optional<srp::ClientResponse>>;

template<typename function_t>
auto async_invoke(function_t func) {
  return std::async(std::launch::async, func);
}

template<typename function_t, typename... args_t>
auto async_invoke(function_t func, args_t... args) {
  return std::async(std::launch::async, func, std::forward<args_t...>(args...));
}

void srp::device_collection::on_start_callback(const std::string &path_template) {
  auto start_function = [this, &path_template](auto &slave) {
    return std::pair<capture_device &, async_response>{slave, async_invoke([this, &path_template, &slave]() {
                                                         return puck_income_message(slave->start_recording(path_template), slave->uid(),
                                                                                    ActionType::start);
                                                       })};
  };

  signal_broadcast(start_function);
}
void srp::device_collection::on_stop_callback() {
  auto stop_function = [this](auto &slave) {
    return std::pair<capture_device &, async_response>{
        slave, async_invoke([this, &slave]() { return puck_income_message(slave->stop_recording(), slave->uid(), ActionType::stop); })};
  };

  signal_broadcast(stop_function);
}
void srp::device_collection::on_sync_callback(size_t sync_point) {
  auto sync_time = [this, &sync_point](auto &slave) {
    return std::pair<capture_device &, async_response>{slave, async_invoke([this, &sync_point, &slave]() {
                                                         return puck_income_message(slave->sync_time(sync_point), slave->uid(),
                                                                                    ActionType::sync_time);
                                                       })};
  };

  signal_broadcast(sync_time);
}
void srp::device_collection::on_state_callback() {
  auto state_trigger_function = [this](auto &slave) {
    return std::pair<capture_device &, async_response>{
        slave, async_invoke([this, &slave]() { return puck_income_message(slave->state(), slave->uid(), ActionType::time); })};
  };

  signal_broadcast(state_trigger_function);
}
void srp::device_collection::on_check_callback() {
  auto check_trigger_function = [this](auto &slave) {
    return std::pair<capture_device &, async_response>{
        slave, async_invoke([this, &slave]() { return puck_income_message(slave->check(), slave->uid(), ActionType::check_device); })};
  };

  signal_broadcast(check_trigger_function);
}

void srp::device_collection::process_result(srp::capture_device &slave, const std::optional<ClientResponse> &response) {
  if (!response) {
    LOGW << "Error in communication with " << slave->uid() << " Exclude from list. ";
    exclude_slave(slave);
  } else {
    auto log_action = srp::ActionMessageBuilder::log_message(response.value());

    if (master_) master_->send_message(log_action);

    std::ranges::for_each(monitors_, [&log_action](auto const &monitor) { monitor->send_message(log_action); });
  }
}

void srp::device_collection::add_capture_device(srp::capture_device device) {
  slaves_.push_back(std::move(device));

  auto check_trigger_function = [this](auto &slave) {
    return std::pair<capture_device &, async_response>{
        slave, std::async(std::launch::deferred,
                          [this, &slave]() { return puck_income_message(slave->check(), slave->uid(), ActionType::check_device); })};
  };

  LOGD << "[UID=" << slaves_.back()->uid() << "] Check new client...";
  signal_to_device(slaves_.back(), check_trigger_function);
  LOGD << "[UID=" << slaves_.back()->uid() << "] Checked...";
}

void srp::device_collection::remove_capture_device(size_t device_id) { exclude_slave(find_device(device_id)); }

srp::capture_device &srp::device_collection::find_device(size_t device_id) {
  auto device = std::find_if(slaves_.begin(), slaves_.end(), [device_id](auto const &dev) { return dev->uid() == device_id; });
  if (device == slaves_.end()) throw std::out_of_range("device id out of range");

  return *device;
}

void srp::device_collection::add_capture_monitor(srp::capture_controller monitor) { monitors_.emplace_back(std::move(monitor)); }
void srp::device_collection::add_master_controller(srp::capture_controller master) {
  if (master_) { master_->stop(); }

  master_ = std::move(master);
  master_->start(controller_callback_set());
}

srp::controller_callbacks srp::device_collection::controller_callback_set() {
  srp::controller_callbacks cbs;
  auto on_start = [this](auto const &action_raw) {
    auto &action = dynamic_cast<srp::ClientActionMessage const &>(action_raw);
    auto data = ProtoUtils::message_from_bytes<ClientStartRecord>(action.meta());
    this->on_start_callback(data.path_pattern());
  };
  cbs.add_callback(ActionType::start, on_start);

  auto on_stop = [this](auto const &action) { this->on_stop_callback(); };
  cbs.add_callback(ActionType::stop, on_stop);

  auto on_sync = [this](auto const &action_raw) {
    auto &action = dynamic_cast<srp::ClientActionMessage const &>(action_raw);
    auto data = ProtoUtils::message_from_bytes<ClientSync>(action.meta());
    this->on_sync_callback(data.sync_point());
  };
  cbs.add_callback(ActionType::sync_time, on_sync);

  auto on_state = [this](auto const &state) { this->on_state_callback(); };
  cbs.add_callback(ActionType::time, on_state);

  cbs.add_callback(ActionType::check_device, [this](auto const &state) { this->on_check_callback(); });
  return cbs;
}
