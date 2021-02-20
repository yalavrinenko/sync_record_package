//
// Created by yalavrinenko on 18.02.2021.
//

#include "sfml_control_instance.hpp"
#include <clients/control/ui_control_client.hpp>
#include <clients/sfml-control/common_info_window.hpp>
#include <clients/sfml-control/device_info_window.hpp>
#include <clients/sfml-control/gui_entries.hpp>
#include <clients/sfml-control/gui_input.hpp>
#include <clients/sfml-control/guilogger.hpp>
#include <numeric>
#include <protocols/actions.hpp>
#include <utils/io.hpp>
#include <utils/logger.hpp>

namespace {
  template<typename message_t>
  std::pair<message_t, size_t> extract_response(auto const &ref) {
    decltype(auto) resp_ref = dynamic_cast<srp::ClientResponse const &>(ref);
    return {srp::ProtoUtils::message_from_bytes<message_t>(resp_ref.data()), resp_ref.uid()};
  }
}

srp::sfml_control_instance::sfml_control_instance(srp::UiControlOption options, std::shared_ptr<gui::logger_window> window):
    opt_(std::move(options)), ui_{std::move(window)}{
  control_ = std::make_unique<srp::ui_control_client>(opt_);

  ui_->register_external_events(sf::Event::EventType::Closed, [this](auto const &ptr){
    control_->stop();
    control_.reset(nullptr);
  });

  main_control_group_ = ui_->create_logger<gui::gui_controls>("Main control buttons");

  create_timers();
  create_control_buttons();
  create_main_info();

  register_callbacks();
}

void srp::sfml_control_instance::register_callbacks() {
  srp::controller_callbacks callbacks;

  auto check_and_add = [this](auto const &id){
    if (!devices_.contains(id)){
      devices_[id] = ui_->create_logger<gui::device_info_window>("Device " + std::to_string(id));
    }
  };

  callbacks.add_callback(check_device, [this, check_and_add](auto const &message) {
    auto [check, who] = extract_response<ClientCheckResponse>(message);

    check_and_add(who);

    auto ci = devices_[who]->current_data();
    ci.active = check.check_ok();
    ci.additional_info = check.info();
    ci.type = check.type();
    ci.name = check.name();
    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  callbacks.add_callback(start, [this, check_and_add](auto const &message) {
    auto [check, who] = extract_response<ClientStartRecordResponse>(message);

    check_and_add(who);

    auto ci = devices_[who]->current_data();
    std::string data_path, split_path;
    for (auto i = 0; i < check.data_path_size(); ++i) {
      data_path += check.data_path(i) + "\n";
      split_path += check.sync_point_path(i) + ";";
    }
    data_path.pop_back(); split_path.pop_back();

    ci.data_path = data_path;
    ci.sync_path = split_path;
    ci.additional_info = "";
    ci.state = gui::device_ui_info::device_state::recording;

    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  callbacks.add_callback(stop, [this, check_and_add](auto const &message) {
    auto [check, who] = extract_response<ClientStopRecordResponse>(message);

    check_and_add(who);

    auto ci = devices_[who]->current_data();
    ci.duration = check.duration_sec();
    ci.frames = check.frames();
    ci.fps = check.average_fps();
    ci.state = gui::device_ui_info::device_state::stopped;

    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  callbacks.add_callback(sync_time, [this, check_and_add](auto const &message) {
    auto [check, who] = extract_response<ClientSyncResponse>(message);

    check_and_add(who);

    auto ci = devices_[who]->current_data();
    ci.sync_point = check.sync_point();
    ci.last_sync_duration = check.duration_sec();
    ci.last_sync_frame = check.frames();
    ci.state = gui::device_ui_info::device_state::synced;

    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  callbacks.add_callback(time, [this, check_and_add](auto const &message) {
    auto [check, who] = extract_response<ClientTimeResponse>(message);

    check_and_add(who);

    auto ci = devices_[who]->current_data();

    ci.active = true;

    ci.duration = check.duration_sec();
    ci.frames = check.frames();
    ci.fps = check.average_fps();
    ci.fps_history.add_data(ci.fps);

    ci.state = gui::device_ui_info::device_state(check.state());

    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  callbacks.add_callback(disconnect, [this, check_and_add](auto const &message) {
    decltype(auto) resp_ref = dynamic_cast<srp::ClientResponse const &>(message);
    auto who = resp_ref.uid();

    check_and_add(who);

    auto ci = devices_[who]->current_data();

    ci.active = false;

    ci.state = gui::device_ui_info::device_state::disconnected;

    devices_[who]->update_info(ci);

    return srp::ClientResponse();
  });

  control_->start(callbacks);
}
void srp::sfml_control_instance::create_control_buttons() {
  main_control_group_->add_control<gui::button_control>("Start recording", [this](auto const&){
    this->start_recording();
  });

  main_control_group_->add_control<gui::button_control>("Stop recording", [this](auto const&){
    this->stop_recording();
  });

  main_control_group_->add_control<gui::button_control>("Start sync. point insertion", [this](auto const&){
    this->start_synchronization();
  });

  main_control_group_->add_control<gui::button_control>("Stop sync. point insertion", [this](auto const&){
    this->stop_synchronization();
  });

  main_control_group_->add_control<gui::button_control>("Check all devices", [this](auto const&){
    control_->send_message(srp::ActionMessageBuilder::check_client());
  });
}

void srp::sfml_control_instance::create_timers() {
  using namespace std::chrono_literals;

  auto status_request = [this](auto &timer){
    ping_action();
    dynamic_cast<gui::timer&>(timer).start();
  };

  status_timer_ = std::make_shared<gui::timer>(200ms, status_request);
  main_control_group_->add_control(status_timer_);

  auto sync_request = [this](auto &timer){
    sync_action();
    dynamic_cast<gui::timer&>(timer).start();
  };
  sync_timer_ = std::make_shared<gui::timer>(std::chrono::milliseconds {opt_.sync_point_interval_millis()}, sync_request);
  main_control_group_->add_control(sync_timer_);
}

void srp::sfml_control_instance::create_main_info() {
  info_ = std::make_shared<gui::common_ui_info>();

  info_->current_sync_point = 0;
  info_->sync_timer = sync_timer_;

  auto main_info_group =  ui_->create_logger<gui::gui_controls>("Recording info");
  main_info_group->add_control<gui::common_info_window>(info_);
}

void srp::sfml_control_instance::start_recording() {
  status_timer_->start();
  info_->start_rec_time_ = std::chrono::high_resolution_clock::now();
  info_->change_state(gui::common_ui_info::recording_state::recording);
  control_->send_message(srp::ActionMessageBuilder::start_message(opt_.record_path_template()));
}

void srp::sfml_control_instance::stop_recording() {
  if (sync_timer_->running())
    stop_synchronization();

  if (info_->state() != gui::common_ui_info::recording_state::recording) {
    LOGE << "Unable to stop recording if not recorded";
    return;
  }

  status_timer_->stop();
  info_->remove_last_state();
  control_->send_message(srp::ActionMessageBuilder::stop_message());
}

void srp::sfml_control_instance::start_synchronization() {
  if (info_->state() != gui::common_ui_info::recording_state::recording) {
    LOGE << "Unable to start synchronization if not recorded";
    return;
  }
  info_->change_state(gui::common_ui_info::recording_state::synchronization);

  info_->current_sync_point = 0;
  sync_action();

  sync_timer_->start();
}

void srp::sfml_control_instance::stop_synchronization() {
  if (info_->state() != gui::common_ui_info::recording_state::synchronization){
    LOGE << "Unable to stop synchronization if not start";
    return;
  }
  info_->current_sync_point = 0;
  sync_action();
  info_->current_sync_point = 0;

  info_->remove_last_state();
  sync_timer_->stop();
}

void srp::sfml_control_instance::sync_action() {
  control_->send_message(srp::ActionMessageBuilder::sync_message(info_->current_sync_point));
  ++info_->current_sync_point;
}
void srp::sfml_control_instance::ping_action() {
  control_->send_message(srp::ActionMessageBuilder::state_request_message());
}

srp::sfml_control_instance::~sfml_control_instance() = default;
