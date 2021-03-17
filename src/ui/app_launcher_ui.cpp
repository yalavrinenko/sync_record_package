//
// Created by yalavrinenko on 17.03.2021.
//
#include "app_runner.hpp"

int main(int argc, char** argv){
  auto gui_factory = gui::logger_environment::create();
  auto main_window = gui_factory->create_logger("Application Launcher");

  srp::app_runner runner(main_window, "sync_server");

  while (main_window->is_open()){
    auto const delay = std::chrono::milliseconds(1000 / 25);
    gui_factory->draw();
    std::this_thread::sleep_for(delay);
  }

  return 0;
}