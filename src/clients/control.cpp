//
// Created by yalavrinenko on 20.01.2021.
//
#include <clients/sfml-control/guilogger.hpp>

int main(int argc, char** argv){
  auto gui_factory = gui::logger_environment::create();
  auto window = gui_factory->create_logger("Ship_S");

  while (true)
    gui_factory->draw();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(12000s);
  return 0;
}