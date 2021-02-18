//
// Created by yalavrinenko on 20.03.2020.
//

#include "gui_entries.hpp"
#include <numeric>
#include <imgui-SFML.h>

void gui::text_entry::draw_impl() {
  std::lock_guard<std::mutex> lg(io_mutex);
  auto &datas_ = text_data_.main();
  for (auto const &[k, v]: datas_)
    ImGui::TextWrapped("%s %s\n", k.c_str(), v.c_str());
}
void gui::numeric_entry::draw_impl() {
  text_entry::draw_impl();

  std::lock_guard<std::mutex> lg(io_mutex);

  auto &idata = numeric_data_.main();
  for (auto const &[k, v] : idata){
    auto &[current, max] = v;
    ImGui::Text("%s %lf\t\t", k.c_str(), current);

    ImGui::ProgressBar(std::abs(current / max));
    ImGui::Text("\n");
  }
}
void gui::moving_plot_entry::draw_impl() {
  std::map<std::string, moving_window> data;
  {
    std::lock_guard lg(io_mutex);
    data = this->data_;
  }
  for (auto &kv : data){
    auto getter = [](void* ptr, int i) -> float{
      return ((std::pair<std::string, moving_window>*)ptr)->second[i];
    };
    auto title = std::to_string(kv.second.back());
    ImGui::Text("%s\n", kv.first.c_str());
    ImGui::PlotLines(title.c_str(), getter, static_cast<void*>(&kv), kv.second.size());
  }
}
gui::histo_plot_entry::histo_plot_entry(
    std::shared_ptr<class logger_window> factory, std::string name)
    : ilogger_entry(std::move(factory), std::move(name)) {}
void gui::histo_plot_entry::flush() {
  std::lock_guard lg(io_mutex);
  data_.swap();
}
void gui::histo_plot_entry::draw_impl() {
  std::lock_guard lg(io_mutex);
  for (auto &kv: data_.main()){
    auto getter = [](void* ptr, int i) -> float {
      auto kv = static_cast<std::pair<std::string, unary_function >*>(ptr);
      return kv->second(i);
    };
    auto color = ImColor(0, 255, 255);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)color);
    //ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, (ImVec4)color);
    ImGui::PlotHistogram(kv.first.c_str(), getter, static_cast<void *>(&kv),
                         kv.second(-1), 0, nullptr, 0, 1, ImVec2(0, 30));
    ImGui::PopStyleColor(1);
  }
}
void gui::histo_plot_entry::log(std::string key,
                                gui::histo_plot_entry::unary_function data) {
  data_.tmp().emplace_back(std::move(key), std::move(data));
}

void gui::polar_entry::flush() {
  std::lock_guard lg(io_mutex);
  data_.swap();
}
void gui::polar_entry::draw_impl() {
  std::lock_guard lg(io_mutex);

  for (auto &kv : data_.main()) {
    ImGui::Columns(2, nullptr, true);
    ImGui::Separator();
    ImGui::SetColumnWidth(0, 135);

    ImDrawList *dlist = ImGui::GetWindowDrawList();
    ImColor color(255, 0, 0);
    const ImVec2 raw_cursor_position = ImGui::GetCursorScreenPos();
    auto const &p = raw_cursor_position;

    float R = 60;
    ImVec2 center = {p.x + R, p.y + R};
    dlist->AddCircle(center, R, color, 100, 1.0f);
    dlist->AddCircleFilled(center, 1, ImColor(255, 255, 255));

    auto n = kv.second(-1);
    for (auto i = 0; i < n.first; ++i){
      auto [x, y] = kv.second(i);
      auto convert = [this](double &r, double &phi){
        if (r >= max_r_)
          r = std::abs(r) / r * max_r_;
        auto x = r * std::cos(phi);
        auto y = r * std::sin(phi);
        r = x; phi = y;
      };
      convert(x, y);
      auto sx = static_cast<float>(x / max_r_) * R;
      auto sy = static_cast<float>(y / max_r_) * R;

      ImVec2 target{center.x + sx, center.y + sy};
      dlist->AddCircleFilled(target, 4, ImColor(255, 255, 255));
      dlist->AddLine(center, target, ImColor(255, 255, 255));
    }

    ImGui::Dummy({120, 120});

    ImGui::NextColumn();
    ImGui::Text("%s\n", kv.first.c_str());
    ImGui::Columns(1);
    ImGui::Separator();
  }
}
gui::polar_entry::polar_entry(std::shared_ptr<class logger_window> factory,
                              std::string name, double max_r)
    : ilogger_entry(std::move(factory), std::move(name)), max_r_{max_r} {}

gui::radar_entry::radar_entry(std::shared_ptr<class logger_window> factory,
                              std::string name, double max_r, size_t segments):
                              ilogger_entry(std::move(factory), std::move(name)),
                              max_r_{max_r}, segments_{segments}{
}
void gui::radar_entry::draw_impl() {
  ImGui::Columns(2, nullptr, true);
  ImGui::Separator();
  ImGui::SetColumnWidth(0, 420);

  ImDrawList *dlist = ImGui::GetWindowDrawList();
  ImColor color(255, 0, 0);
  const ImVec2 raw_cursor_position = ImGui::GetCursorScreenPos();
  auto const &p = raw_cursor_position;

  float R = 200;
  float dR = R / segments_;
  ImVec2 center = {p.x + R, p.y + R};
//
//  dlist->AddLine({center.x - R, center.y}, {center.x + R, center.y}, color, 1);
//  dlist->AddLine({center.x, center.y - R}, {center.x, center.y + R}, color, 1);

  dlist->AddCircleFilled(center, 1, ImColor(255, 255, 255));
  for (auto i = 1u; i <= segments_; ++i) {
    dlist->AddCircle(center, dR * i, color, 100, 1.0f);
  }

  for (auto &point : data_.main()){
    auto x = point.r; auto y = point.phi;

    auto convert = [this](double &r, double &phi){
      if (r >= max_r_)
        r = std::abs(r) / r * max_r_;
      auto x = r * std::cos(phi);
      auto y = r * std::sin(phi);
      r = x; phi = y;
    };
    convert(x, y);
    auto sx = static_cast<float>(x / max_r_) * R;
    auto sy = static_cast<float>(y / max_r_) * R;

    ImVec2 target{center.x + sx, center.y + sy};
    dlist->AddCircleFilled(target, 4, ImColor(255, 255, 255));
    dlist->AddRect({target.x-5, target.y-5}, {target.x+5, target.y+5}, ImColor{0, 255,0}, 2);
    //dlist->AddLine(center, target, ImColor(255, 255, 255));
  }

  ImGui::Dummy({400, 400});

  ImGui::NextColumn();
  for (auto &point : data_.main()){
    ImGui::Text("Target %s distance %0.6lf\n", point.description.c_str(), point.r);
  }
  ImGui::Columns(1);
  ImGui::Separator();
}
