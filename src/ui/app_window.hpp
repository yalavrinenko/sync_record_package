//
// Created by yalavrinenko on 17.03.2021.
//

#ifndef SRP_APP_WINDOW_HPP
#define SRP_APP_WINDOW_HPP

#include <clients/sfml-control/gui_input.hpp>
#include <filesystem>
#include <boost/asio.hpp>

namespace gui {
  using on_close_t = std::function<void(ilogger_entry*)>;

  class app_window: public icontrol {
  public:
    explicit app_window(std::filesystem::path bin, std::filesystem::path json, on_close_t = nullptr);

    void draw() override;

    [[nodiscard]] auto is_running() const { return is_started_; }

    void link_window(auto *window) { linked_window_ = window; }

    ~app_window() override;
  private:
    class app_instance;

    std::unique_ptr<app_instance> app_;

    std::filesystem::path runner_;
    std::filesystem::path json_;

    bool is_started_ = false;

    float last_scroll_position_ = 0.f;
    on_close_t on_close_;
    gui::ilogger_entry* linked_window_ = nullptr;
  };
}


#endif//SRP_APP_WINDOW_HPP
