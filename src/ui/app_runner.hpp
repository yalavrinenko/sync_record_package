//
// Created by yalavrinenko on 17.03.2021.
//

#ifndef SRP_APP_RUNNER_HPP
#define SRP_APP_RUNNER_HPP

#include <clients/sfml-control/guilogger.hpp>
#include <clients/sfml-control/gui_entries.hpp>
#include <clients/sfml-control/gui_input.hpp>
#include "app_window.hpp"


namespace srp {
  class app_runner {
  public:
    explicit app_runner(auto window, std::filesystem::path runner): window_{std::move(window)}{
      auto server_window = window_->create_logger<gui::gui_controls>("Sync. server");
      server_window->add_control<gui::app_window>(std::move(runner));
    }

  protected:
    std::shared_ptr<gui::logger_window> window_;
  };
}



#endif//SRP_APP_RUNNER_HPP
