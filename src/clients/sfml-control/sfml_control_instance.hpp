//
// Created by yalavrinenko on 18.02.2021.
//

#ifndef SRP_SFML_CONTROL_INSTANCE_HPP
#define SRP_SFML_CONTROL_INSTANCE_HPP
#include <options.pb.h>
#include <unordered_map>

namespace gui{
  class logger_window;
  class timer;
  class gui_controls;
  class common_ui_info;
  class device_info_window;
  class button_control;
}

namespace srp {
  class ui_control_client;
  class sfml_control_instance {
  public:
    sfml_control_instance(srp::UiControlOption options, std::shared_ptr<gui::logger_window> window);

    ~sfml_control_instance();
  protected:
    void register_callbacks();

    void create_control_buttons();

    void create_timers();

    void create_main_info();

    void start_recording();
    void stop_recording();

    void start_synchronization();
    void stop_synchronization();

    void sync_action();
    void ping_action();

  private:
    srp::UiControlOption opt_;
    std::unique_ptr<srp::ui_control_client> control_;
    std::shared_ptr<gui::logger_window> ui_;

    std::shared_ptr<gui::gui_controls> main_control_group_;

    std::shared_ptr<gui::timer> status_timer_;

    std::shared_ptr<gui::timer> sync_timer_;

    std::shared_ptr<gui::common_ui_info> info_;

    std::unordered_map<size_t, std::shared_ptr<gui::device_info_window>> devices_;

    enum class buttons_type{
      start, stop, start_sync, stop_sync, check
    };

    std::unordered_map<buttons_type, std::shared_ptr<gui::button_control>> buttons_;
  };
}


#endif//SRP_SFML_CONTROL_INSTANCE_HPP
