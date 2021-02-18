//
// Created by yalavrinenko on 20.01.2021.
//
#include <clients/sfml-control/guilogger.hpp>
#include <utils/logger.hpp>
#include <utils/io.hpp>
#include <clients/sfml-control/sfml_control_instance.hpp>

class UiWindow{
public:
  static auto start_ui(std::string const &name);
  static void wait();
private:
  static auto& w_thread() {
    static std::future<void> working_thread;
    return working_thread;
  }
  static auto& factory() {
    static auto gui_factory = gui::logger_environment::create();
    return gui_factory;
  }
  static auto& window(auto const &name) {
    static auto win = factory()->create_logger(name);
    return win;
  }
  static bool stop_iu_;
};
bool UiWindow::stop_iu_{false};

auto UiWindow::start_ui(std::string const &name) {
  auto active_window = UiWindow::window(name);

  return active_window;
}

void UiWindow::wait() {
  using namespace std::chrono_literals;
  while (!UiWindow::stop_iu_) {
    std::this_thread::sleep_for(10ms);
    UiWindow::factory()->draw();
  }
}

int main(int argc, char** argv){
  if (argc < 2) {
    LOGE << "Option file is required. Use ./app option.json.";
    return -1;
  }
  auto json = srp::IoUtils::read_json(argv[1]);
  auto options = srp::ProtoUtils::message_from_json<srp::UiControlOption>(json);

  if (!options) { return -1; }

  auto window = UiWindow::start_ui("Sync. recording control");

  srp::sfml_control_instance instance(options.value(), window);

  UiWindow::wait();
  return 0;
}