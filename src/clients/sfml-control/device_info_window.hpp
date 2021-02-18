//
// Created by yalavrinenko on 18.02.2021.
//

#ifndef SRP_DEVICE_INFO_WINDOW_HPP
#define SRP_DEVICE_INFO_WINDOW_HPP

#include <memory>
#include <clients/sfml-control/gui_input.hpp>
#include <clients/sfml-control/moving_window.hpp>

namespace gui {
  struct device_ui_info{
    unsigned long device_id{};
    std::string type{};
    std::string name{};
    bool active{false};

    std::string additional_info{};

    std::string data_path{};
    std::string sync_path{};

    double duration{};
    unsigned long frames{};
    double fps{};
    moving_window fps_history{100};


    long sync_point{};
    double last_sync_duration{};
    long unsigned last_sync_frame{};

    enum class device_state{
      undefined = 0,
      stopped = 1,
      recording = 2,
      synced  = 3,
      disconnected = 4
    };

    device_state state{device_state::undefined};
  };

  class device_info_window: public gui::ilogger_entry {
  public:
    explicit device_info_window(std::shared_ptr<logger_window> factory, std::string name);
    void flush() override;

    device_ui_info current_data();

    void update_info(gui::device_ui_info info);

  protected:
    void draw_impl() override;

  private:
    gui::cloned_data<device_ui_info> data_;
  };
}


#endif//SRP_DEVICE_INFO_WINDOW_HPP
