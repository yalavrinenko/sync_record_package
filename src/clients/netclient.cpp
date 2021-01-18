//
// Created by yalavrinenko on 12.01.2021.
//

#include "netclient.hpp"
#include "../capture_interface/capture.hpp"
#include "../net/netcomm.hpp"
#include "../net/sessions.hpp"
#include "../utils/io.hpp"
#include "../utils/logger.hpp"
#include "options.pb.h"
#include <boost/asio.hpp>
#include <future>
#include <ranges>
#include <unordered_map>

class srp::netclient::instance_handler {
public:
  explicit instance_handler(std::unique_ptr<srp::capture_i> capture, ControlServerOption const &target_opt)
      : device_{std::move(capture)}, socket_(ios_) {
    socket_.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(target_opt.host()), target_opt.port()));
    if (socket_.is_open()) {
      auto comm = std::make_unique<netcomm>(std::move(socket_));
      session_ = base_session::create_session(std::move(comm));
      is_active_ = session_->is_active();

      if (is_active()) {
        auto session_type = ClientWelcomeMessage();
        session_type.set_type(client);
        session_->send_message(session_type);

        auto registration = session_->receive_message<ClientRegistrationMessage>();
        if (registration) {
          uid_ = registration->uid();
        } else {
          LOGE << "Fail to get uid.";
          is_active_ = false;
        }
      }
    } else {
      LOGE << "Unable to connect to control server.";
      is_active_ = false;
    }
  }

  void run() {
    stop_main_loop = false;
    main_loop();
  }

  void stop() { stop_main_loop = true; }

  [[nodiscard]] bool is_active() const { return is_active_ && session_ && session_->is_active(); }

protected:
  template<typename proto_t>
  auto serialize_response(std::optional<proto_t> const &s) {
    std::string resp{};
    if (s) { s->SerializeToString(&resp); }
    return resp;
  }

  std::string execute_command(ClientActionMessage const &command) {
    std::unordered_map<ActionType, std::function<std::string(void)>> actions{
        {ActionType::check_device, [this, &command]() { return serialize_response(device_->check()); }},
        {ActionType::start,
         [this, &command]() {
           auto meta = ProtoUtils::message_from_bytes<ClientStartRecord>(command.meta());
           return serialize_response(device_->start_recording(meta.path_pattern()));
         }},
        {ActionType::stop, [this, &command]() { return serialize_response(device_->stop_recording()); }},
        {ActionType::sync_time,
         [this, &command]() {
           auto meta = ProtoUtils::message_from_bytes<ClientSync>(command.meta());
           return serialize_response(device_->sync_time(meta.sync_point()));
         }},
        {ActionType::time, [this, &command]() { return serialize_response(device_->state()); }},
        {ActionType::disconnect, [this, &command]() { device_->stop_recording(); is_active_ = false; return ""; }}
    };

    return actions[command.action()]();
  }

  [[nodiscard]] ClientResponse create_response(ActionType trigger, const std::string& data) const {
    ClientResponse resp;
    resp.set_uid(uid_);
    resp.set_trigger_action(trigger);
    resp.set_data(data);
    return resp;
  }

private:
  void main_loop() {
    while (!stop_main_loop && is_active()) {
      auto command = session_->receive_message<ClientActionMessage>();
      if (command) {
        auto response = execute_command(command.value());
        if (session_->is_active())
          session_->send_message(create_response(command->action(), response));
      } else {
        LOGE << "Fail to parse command.";
      }
    }
  }

  std::unique_ptr<srp::capture_i> device_;
  std::shared_ptr<srp::base_session> session_ = nullptr;
  bool is_active_{false};

  std::atomic<bool> stop_main_loop;

  boost::asio::io_service ios_;
  boost::asio::ip::tcp::socket socket_;
  size_t uid_;
};

srp::netclient::~netclient() = default;

void srp::netclient::stop() {
  std::ranges::for_each(instances_, [](auto &instance) { instance->stop(); });
}

void srp::netclient::run() {
  auto exec = [](auto &handler) { return std::async(std::launch::async, [&handler]() { handler->run(); }); };

  std::vector<std::future<void>> wthreads;
  std::ranges::transform(instances_, std::back_inserter(wthreads), exec);
  std::ranges::for_each(wthreads, [](auto &thread) { thread.get(); });
}

[[maybe_unused]] bool srp::netclient::add_client_instance(std::unique_ptr<srp::capture_i> capture, ControlServerOption const &target_opt) {
  auto check = capture->check();

  if (check && check->check_ok()) {
    instances_.emplace_back(std::make_unique<instance_handler>(std::move(capture), target_opt));
    LOGD << "Add capture client to instances set.";
    return true;
  } else {
    LOGD << "Check fail. Reason: " << check->info();
    return false;
  }
}

size_t srp::netclient::instances_count() const {
  return instances_.size();
}

srp::netclient::netclient() = default;
