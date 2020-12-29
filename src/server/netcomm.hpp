//
// Created by yalavrinenko on 29.12.2020.
//

#ifndef SRP_NETCOMM_HPP
#define SRP_NETCOMM_HPP
#include "../capture_interface/communication_i.hpp"
#include <boost/asio.hpp>

namespace srp{
  class netcomm: public communication_i{
  public:
    explicit netcomm(boost::asio::ip::tcp::socket socket);

    bool send_message(const google::protobuf::MessageLite *message) override;
    bool receive_message(google::protobuf::MessageLite *message) override;
    bool is_alive() override;
    std::string commutator_info() override;
    ~netcomm() override;

  protected:
    struct netcomm_impl;
    std::unique_ptr<netcomm_impl> pimpl_;
  };
}
#endif//SRP_NETCOMM_HPP
