//
// Created by yalavrinenko on 29.12.2020.
//
#include "../utils/logger.hpp"
#include "netcomm.hpp"
#include <google/protobuf/util/delimited_message_util.h>
struct srp::netcomm::netcomm_impl{
  boost::asio::ip::tcp::iostream tcp_stream;
  explicit netcomm_impl(boost::asio::ip::tcp::socket socket): tcp_stream{std::move(socket)}{
  }

  bool send_message(google::protobuf::MessageLite const* message) {
    u_int64_t message_size = message->ByteSizeLong();
    std::string message_string;
    if (message->SerializeToString(&message_string)) {
      tcp_stream << message_size << message_string;
      tcp_stream.flush();
      return bool(tcp_stream);
    } else {
      LOGW << "Unable to serialize message";
      return false;
    }
  }

  bool recieve_message(google::protobuf::MessageLite *message){
    u_int64_t message_size;
    if (tcp_stream >> message_size){
      std::vector<char> buffer(message_size);
      auto extracted = tcp_stream.readsome(buffer.data(), message_size);
      if (extracted == static_cast<long>(message_size)){
        if (message->ParsePartialFromArray(buffer.data(), message_size)){
          return true;
        } else {
          LOGW << "Fail to parse income data";
        }
      } else {
        LOGW << "Read " << extracted << " bytes but expected " << message_size;
      }
    } else {
      LOGW << "Fail to read message size. Reason: " << tcp_stream.error().message();
    }
    return false;
  }

  bool is_alive() const {
    return bool(tcp_stream);
  }

  auto address() {
    if (tcp_stream)
      return tcp_stream.socket().remote_endpoint().address().to_string();
    else
      return std::string{"Unknown"};
  }

  auto port(){
    if (tcp_stream)
      return std::to_string(tcp_stream.socket().remote_endpoint().port());
    else
      return std::string{"Unknown"};
  }

  void terminate() {
    //tcp_stream.expires_after(std::chrono::milliseconds(10));
  }
};

bool srp::netcomm::send_message(const google::protobuf::MessageLite *message) {
  return pimpl_->send_message(message);
}
bool srp::netcomm::receive_message(google::protobuf::MessageLite *message) {
  return pimpl_->recieve_message(message);
}
bool srp::netcomm::is_alive() {
  return pimpl_->is_alive();
}
std::string srp::netcomm::commutator_info() {
  return "Address: " + pimpl_->address() + ":" + pimpl_->port();
}

srp::netcomm::netcomm(boost::asio::ip::tcp::socket socket): pimpl_{std::make_unique<srp::netcomm::netcomm_impl>(std::move(socket))} {
}
void srp::netcomm::terminate() {
  pimpl_->terminate();
}

srp::netcomm::~netcomm() = default;