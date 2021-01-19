//
// Created by yalavrinenko on 25.12.2020.
//

#ifndef SRP_CAPTURE_CONTROL_HPP
#define SRP_CAPTURE_CONTROL_HPP
#include <memory>
#include <functional>
#include "session.pb.h"
#include <variant>

namespace srp{

  struct controller_callbacks{
    using control_callback = std::function<void(srp::ClientActionMessage const&)>;

    void add_callback(srp::ActionType action, control_callback const& cb){
      callbacks_[action] = cb;
    }

    auto const& callback_map() const { return callbacks_; }

  private:
    std::map<ActionType, control_callback> callbacks_;
  };

  class controller_i{
  public:
    virtual void start(const controller_callbacks &callbacks) = 0;

    virtual void stop() = 0;

    virtual void send_message(srp::ClientActionMessage const& crp) = 0;

    virtual ~controller_i() = default;
  };

  using capture_controller = std::unique_ptr<controller_i>;
}
#endif//SRP_CAPTURE_CONTROL_HPP
