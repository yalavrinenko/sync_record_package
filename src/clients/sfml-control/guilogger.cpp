//
// Created by yalavrinenko on 19.03.2020.
//

#include "guilogger.hpp"
#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

ImGuiContext* create_imgui_context(){
  static auto* atlas = new ImFontAtlas();
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

    for (auto &entry : entries_)
      if (entry) {
        entry->draw();
      }

    window_.clear(); // fill background with color
    ImGui::SFML::Render(window_);
    window_.display();
}
void gui::logger_window::events() {
  sf::Event event{};
  while (window_.pollEvent(event)) {
    ImGui::SFML::ProcessEvent(event);

    if (event.type == sf::Event::Closed) {
      window_.close();
      std::exit(0);
    }
  }
}

gui::ilogger_entry::ilogger_entry(std::shared_ptr<class logger_window> factory, std::string name):
    linked_factory_{std::move(factory)}, name_{std::move(name)}{}
void gui::ilogger_entry::draw() {
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
  for (auto &window : windows_){
    if (window->is_open()){
      window->flush();
    }
  }
}
