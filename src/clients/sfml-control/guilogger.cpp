//
// Created by yalavrinenko on 19.03.2020.
//

#include "guilogger.hpp"
#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

ImGuiContext *create_imgui_context() {
  static auto *atlas = new ImFontAtlas();
  return ImGui::CreateContext(atlas);
}

void gui::logger_window::init_window() {
  window_.setVerticalSyncEnabled(true);
  ctx_ = create_imgui_context();
  ImGui::SFML::Init(window_);
  window_.resetGLStates();
  window_.setFramerateLimit(60);
}

gui::logger_window::~logger_window() {
  ImGui::DestroyContext(ctx_);
  ImGui::SFML::Shutdown();
}

void gui::logger_window::draw() {
  events();

  ImGui::SFML::Update(window_, delta_clock_.restart());

  auto font = ImGui::GetFont();
  font->FontSize = 18;

  ImGui::PushFont(font);

  for (auto &entry : entries_)
    if (entry) { entry->draw(); }

  ImGui::PopFont();

  window_.clear();// fill background with color
  ImGui::SFML::Render(window_);
  window_.display();

  if (!exclude_entries_.empty()) {
    for (auto const &ptr : exclude_entries_) { entries_.remove_if([ptr](auto const& x) { return x.get() == ptr; }); }
    exclude_entries_.clear();
  }
}

void gui::logger_window::register_external_events(sf::Event::EventType event, event_callback cb) { callbacks_[event] = std::move(cb); }

void gui::logger_window::events() {
  sf::Event event{};
  while (window_.pollEvent(event)) {
    ImGui::SFML::ProcessEvent(event);

    if (event.type == sf::Event::Closed) {
      window_.close();
      call_callback(event.type);
      std::exit(0);
    }
  }
}

gui::ilogger_entry::ilogger_entry(std::shared_ptr<class logger_window> factory, std::string name)
    : linked_factory_{std::move(factory)}, name_{std::move(name)} {}
void gui::ilogger_entry::draw() {
  if (name_.find("Device") != std::string::npos) {
    ImGui::SetNextWindowSize({400, 420}, ImGuiCond_FirstUseEver); //ImGuiCond_FirstUseEver
  }

  ImGui::Begin(name_.c_str());
  draw_impl();
  ImGui::End();
}

void gui::logger_environment::draw() {
  // while (!is_stop_sig_) {
  for (auto &window : windows_) {
    if (window->is_open()) {
      window->flush();
      window->draw();
    }
  }
  // }
}
void gui::logger_environment::flush() {
  for (auto &window : windows_) {
    if (window->is_open()) { window->flush(); }
  }
}
