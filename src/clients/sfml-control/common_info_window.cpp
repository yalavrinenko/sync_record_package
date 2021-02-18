//
// Created by yalavrinenko on 18.02.2021.
//

#include "common_info_window.hpp"
#include <utils/io.hpp>

void gui::common_info_window::draw() {
  ImGui::Text("Recording state: %s\n", info_->state_str().c_str());
  ImGui::Text("Recording time: %s\n", srp::TimeDateUtils::duration_to_str(info_->recording_time()).c_str());

  ImGui::Separator();

  ImGui::Text("Current sync. point: %lu\n", info_->current_sync_point);

  if (info_->sync_timer) {
    ImGui::Text("Recording time from last point: %s\n", srp::TimeDateUtils::duration_to_str(info_->sync_timer->elapsed()).c_str());

    ImGui::ProgressBar(static_cast<float>(info_->sync_timer->elapsed().count()) / info_->sync_timer->period().count(), {0.f, 1.f});
  }
}
std::string gui::common_ui_info::state_str() const {
  switch (state()) {
    case recording_state::recording: return "Recording...";
    case recording_state::synchronization: return "Synchronization...";
    case recording_state::wait: return "Ready to use";
    case recording_state::check_devices: return "Checking...";
  }
  return "Undefined";
}
