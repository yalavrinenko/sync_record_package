//
// Created by yalavrinenko on 29.12.2020.
//

#ifndef SRP_COMMUNICATION_I_HPP
#define SRP_COMMUNICATION_I_HPP
#include <string>
namespace google{
  namespace protobuf{
    class MessageLite;
  }
}

namespace srp {

  class communication_i {
  public:
    virtual bool send_message(const google::protobuf::MessageLite *message) = 0;
    virtual bool receive_message(google::protobuf::MessageLite* message) = 0;
    virtual bool is_alive() = 0;
    virtual std::string commutator_info() = 0;
    virtual void terminate() = 0;
    virtual ~communication_i() = default;

  };
}


#endif//SRP_COMMUNICATION_I_HPP

