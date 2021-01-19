//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_ACTIONS_HPP
#define SRP_ACTIONS_HPP
#include <session.pb.h>

namespace srp{
  struct ActionMessageBuilder{
    static auto empty_message(ActionType action){
      ClientActionMessage message; message.set_action(action);
      return message;
    };

    static auto disconnect_action() {
      return empty_message(ActionType::disconnect);
    }

    static auto register_client(size_t uid) {
      auto message = empty_message(ActionType::registration);
      auto meta = ClientRegistrationMessage();
      meta.set_uid(uid);

      message.set_meta(meta.SerializeAsString());

      return message;
    }

    static auto check_client() {
      return empty_message(ActionType::check_device);
    }

    static auto start_message(const std::string &path_template) {
      auto message = empty_message(ActionType::start);
      auto meta = ClientStartRecord(); meta.set_path_pattern(path_template);
      message.set_meta(meta.SerializeAsString());
      return message;
    }

    static auto stop_message() {
      return empty_message(ActionType::stop);
    }

    static auto state_request_message() {
      return empty_message(ActionType::time);
    }

    static auto sync_message(size_t sync_point){
      auto message = empty_message(ActionType::sync_time);
      ClientSync meta; meta.set_sync_point(sync_point);
      message.set_meta(meta.SerializeAsString());
      return message;
    }

    static auto log_message(ClientResponse const &crp){
      auto message = empty_message(ActionType::log);
      message.set_meta(crp.SerializeAsString());
      return message;
    }
  };
}

#endif//SRP_ACTIONS_HPP
