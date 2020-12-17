//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_ACTIONS_HPP
#define SRP_ACTIONS_HPP
#include <session.pb.h>

namespace srp{
  struct ActionMessageBuilder{
    static ClientActionMessage disconnect_action() {
      auto message = ClientActionMessage();
      message.set_action(ActionType::disconnect);
      return message;
    }

    static ClientActionMessage register_client(size_t uid) {
      auto message = ClientActionMessage();
      message.set_action(ActionType::registration);
      auto meta = ClientRegistrationMessage();
      meta.set_uid(uid);

      message.set_meta(meta.SerializeAsString());
      return message;
    }
  };
}

#endif//SRP_ACTIONS_HPP
