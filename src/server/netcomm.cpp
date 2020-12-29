//
// Created by yalavrinenko on 29.12.2020.
//
#include "netcomm.hpp"
#include <google/protobuf/util/delimited_message_util.h>
struct srp::netcomm::netcomm_impl{
  boost::asio::ip::tcp::iostream tcp_stream;
  google::protobuf::io::IstreamInputStream proto_istream;
  bool istream_eof = false;

  explicit netcomm_impl(boost::asio::ip::tcp::socket socket): tcp_stream{std::move(socket)}, proto_istream(&tcp_stream){
  }

  bool send_message(google::protobuf::MessageLite const* message) {
    google::protobuf::util::SerializeDelimitedToOstream(*message, &tcp_stream);
    tcp_stream.flush();
    return bool(tcp_stream);
  }

  bool recieve_message(google::protobuf::MessageLite *message){
    return google::protobuf::util::ParseDelimitedFromZeroCopyStream(message, &proto_istream, &istream_eof);
  }

  bool is_alive() const {
    return tcp_stream || !istream_eof;
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

srp::netcomm::~netcomm() = default;