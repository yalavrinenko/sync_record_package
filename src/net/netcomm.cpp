//
// Created by yalavrinenko on 29.12.2020.
//
#include "netcomm.hpp"
#include "utils/logger.hpp"
#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>

#include <session.pb.h>

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }
  return ret;
}

auto to_json(auto const* message) {
  std::string str;
  auto status = google::protobuf::util::MessageToJsonString(dynamic_cast<google::protobuf::Message const&>(*message), &str);
  return str;
};

auto to_base64(auto const& data){
  return base64_encode(data.data(), data.size());
}

//--------------------------------------------------------------------------------------------------------------------------------------------

struct srp::netcomm::netcomm_impl{
  boost::asio::ip::tcp::iostream tcp_stream;
  explicit netcomm_impl(boost::asio::ip::tcp::socket socket): tcp_stream{std::move(socket)}{
  }

  bool send_message(const google::protobuf::Message *message) {
    size_t message_size = message->ByteSizeLong();

    std::vector<uint8_t> message_string(message_size);
    if (message->SerializeToArray(message_string.data(), message_string.size())) {
      tcp_stream.write(reinterpret_cast<const char *>(&message_size), sizeof(message_size));
      tcp_stream.write(reinterpret_cast<const char *>(message_string.data()), message_size);
      tcp_stream.flush();

//      LOGD << "Send: " << message_size << " bytes. Content: " << to_base64(message_string);

      //tcp_stream << message_size << message_string;
      return bool(tcp_stream);
    } else {
      LOGW << "Unable to serialize message";
      return false;
    }
  }

  bool recieve_message(google::protobuf::Message *message){
    size_t message_size;

//    if (tcp_stream >> message_size){
    if (tcp_stream.read(reinterpret_cast<char *>(&message_size), sizeof(message_size))){
      std::vector<uint8_t> buffer(message_size);

      tcp_stream.read(reinterpret_cast<char *>(buffer.data()), message_size);

      auto extracted = buffer.size();
      if (extracted == message_size){
        if (message->ParseFromArray(buffer.data(), message_size)){

//          LOGD << "Receive: " << message_size << " bytes. Content: " << to_base64(buffer);

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
    boost::system::error_code ecode;
    auto &socket = tcp_stream.socket();
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ecode);
    LOGW << "Read timeout. Shutdown read operation. " << ecode.message();
    socket.close(ecode);
    LOGW << "Close socket. " << ecode.message();
  }
};

bool srp::netcomm::send_message(const google::protobuf::Message *message) {
  return pimpl_->send_message(message);
}
bool srp::netcomm::receive_message(google::protobuf::Message *message) {
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