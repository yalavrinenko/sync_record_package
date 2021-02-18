//
// Created by yalavrinenko on 18.02.2021.
//

#ifndef SRP_COMMON_INFO_WINDOW_HPP
#define SRP_COMMON_INFO_WINDOW_HPP
#include <clients/sfml-control/gui_input.hpp>
#include <utility>
#include <stack>

namespace gui {
  struct common_ui_info{
    unsigned long current_sync_point{0};
    std::shared_ptr<gui::timer> sync_timer{nullptr};

    enum class recording_state {
      recording,
      synchronization,
      wait,
      check_devices
    };

    std::stack<recording_state> state_q;

    void change_state(recording_state next_state) { state_q.push(next_state); }
    void remove_last_state() { if (!state_q.empty()) state_q.pop(); }

    std::chrono::high_resolution_clock::time_point start_rec_time_;

    std::string state_str() const;

    auto state() const { return (state_q.empty()) ? recording_state::wait : state_q.top(); }

    [[nodiscard]] auto recording_time() const {
      if (state() != recording_state::wait && state() != recording_state::check_devices)
        return std::chrono::high_resolution_clock::now() - start_rec_time_;
      else
        return start_rec_time_ - start_rec_time_;
    }
  };
  class common_info_window: public icontrol {
  public:
    explicit common_info_window(std::shared_ptr<common_ui_info> info): info_{std::move(info)} {}

    void draw() override;

  protected:
    std::shared_ptr<common_ui_info> info_;
  };
}


#endif//SRP_COMMON_INFO_WINDOW_HPP
