//
// Created by yalavrinenko on 18.02.2021.
//

#include "device_info_window.hpp"
#include <clients/sfml-control/gui_entries.hpp>
#include <utility>

gui::device_info_window::device_info_window(std::shared_ptr<logger_window> factory, std::string name)
    : ilogger_entry(std::move(factory), std::move(name)) {
  data_.main().emplace_back(device_ui_info{});
}
void gui::device_info_window::flush() {
  std::lock_guard lg(io_mutex);
  data_.swap();
}
void gui::device_info_window::update_info(gui::device_ui_info info) {
  data_.tmp().emplace_back(std::move(info));
}
void gui::device_info_window::draw_impl() {
  std::lock_guard lg{io_mutex};

  if (data_.main().empty())
    return;

  auto &info = data_.main()[0];

  ImVec4 entry_color ={0, 0, 100, 255};

  auto key_value_print = [&](auto const& key, auto const &value){
    ImGui::TextColored(entry_color, "%s", key);
    ImGui::SameLine(0, 0);
    ImGui::TextWrapped(" %s", value);
  };

  key_value_print("Device id:", std::to_string(info.device_id).c_str());
  key_value_print("Device type:", info.type.c_str());
  key_value_print("Device name:", info.name.c_str());
  ImGui::TextColored(entry_color, "Device state: "); ImGui::SameLine(0, 0);
  if (info.state == device_ui_info::device_state::disconnected){
    ImGui::TextColored({255, 0, 0, 255}, "Disconnected");
  } else {
    if (info.active) ImGui::TextColored({0, 255, 0, 255}, "Ready");
    else
      ImGui::TextColored({255, 0, 0, 255}, "Undefined");
  }

  ImGui::Separator();

  if (!(info.state == device_ui_info::device_state::recording || info.state == device_ui_info::device_state::synced)){
    key_value_print("Recorded:", "");
  } else
    key_value_print("Recording...", "");

  key_value_print("\tDuration [seconds]:", std::to_string(info.duration).c_str());
  key_value_print("\tFrames:", std::to_string(info.frames).c_str());
  key_value_print("\tAverage fps :", std::to_string(info.fps).c_str());

  if (info.state == device_ui_info::device_state::recording || info.state == device_ui_info::device_state::synced) {
    ImGui::PlotLines("Fps timeline", &(info.fps_history[0]), info.fps_history.size());
  }

  if (!info.data_path.empty()){
    key_value_print("Raw data path:", "");
    ImGui::TextWrapped("%s", info.data_path.c_str());

    key_value_print("Timestamps path:", "");
    ImGui::TextWrapped("%s", info.sync_path.c_str());
  }

  ImGui::Separator();
  //if (info.sync_point != 0){
  key_value_print("Last sync. point: ", std::to_string(info.sync_point).c_str());
  ImGui::Text("Sync point at %lf sec. and %lu frame", info.last_sync_duration, info.last_sync_frame);
  //}

  ImGui::Separator();
  if (!info.additional_info.empty())
    key_value_print("Additional info:", info.additional_info.c_str());

//  ImGui::Separator();
//  if (ImGui::Button("X")){
//    this->linked_factory_->remove_logger(this);
//  }
}
gui::device_ui_info gui::device_info_window::current_data() {
  std::lock_guard lg{io_mutex};
  return data_.main().front();
}
