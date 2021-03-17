//
// Created by yalavrinenko on 17.03.2021.
//

#ifndef SRP_APP_RUNNER_HPP
#define SRP_APP_RUNNER_HPP

#include "app_window.hpp"
#include <clients/sfml-control/gui_entries.hpp>
#include <clients/sfml-control/gui_input.hpp>
#include <clients/sfml-control/guilogger.hpp>


namespace srp {
  class app_runner : public gui::icontrol {
  public:
    explicit app_runner(auto window) : window_{std::move(window)} {
    }

    void draw() override;

    ~app_runner() override = default;

  protected:
    std::shared_ptr<gui::logger_window> window_;

    std::array<std::string_view, 3> const buttons_{"Start Server App.",
                                                   "Start Control/Monitor App.",
                                                   "Start Capture App."};
    size_t window_uid_ = 0;
#ifdef WIN32
    std::array<std::string_view, 3> const runner_set_{"sync_server.exe",
                                                      "control_client.exe",
                                                      "capture_client.exe"};
#else
    std::array<std::string_view, 3> const runner_set_{"sync_server",
                                                      "control_client",
                                                      "capture_client"};
#endif
  };
}// namespace srp


#endif//SRP_APP_RUNNER_HPP
