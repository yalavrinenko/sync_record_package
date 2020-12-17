//
// Created by yalavrinenko on 10.12.2020.
//

#ifndef SRP_IO_HPP
#define SRP_IO_HPP

#include "logger.hpp"
#include <boost/asio.hpp>

namespace srp {
  struct NetUtils {

    struct IoParameters{
      static constexpr auto READ_TIMEOUT() {
        using namespace std::chrono_literals;
        return 5s;
      }

      static constexpr auto MAX_DATA_BYTE_SIZE() {
        return 2048;
      }
    };

    template<typename buffer_t, typename stream_t, typename duration_t>
    static auto sync_read_message(stream_t &socket, buffer_t const &buffer, duration_t timeout){

      auto handler = [](stream_t &stream, buffer_t const &buf){
        boost::system::error_code ecode;
        boost::asio::read(stream, boost::asio::buffer(buf), ecode);
        return ecode;
      };

      auto read_future = std::async(std::launch::async, handler, std::ref(socket), std::cref(buffer));
      auto wait = read_future.wait_for(timeout);

      if (wait == std::future_status::timeout){
        boost::system::error_code ecode;
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ecode);
        LOGW << "Read timeout. Shutdown read operation. " << ecode.message();
        socket.close(ecode);
        LOGW << "Close socket. " << ecode.message();
        return false;
      }

      auto ecode = read_future.get();

      if (ecode){
        LOGW << "Unable to read from " << socket.remote_endpoint().address().to_string() << ". Reason: "
             << ecode.message();
        return false;
      }

      return true;
    }

    template<typename buffer_t, typename stream_t>
    static auto sync_read_message(stream_t &socket, buffer_t const &buffer){
      boost::system::error_code ecode;
      boost::asio::read(socket, boost::asio::buffer(buffer), ecode);

      if (ecode){
        LOGW << "Unable to read from socket" << ". Reason: "
             << ecode.message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename stream_t>
    static std::optional<proto_t> sync_read_proto(stream_t &socket){
      size_t size;

      if (sync_read_message(socket, boost::asio::buffer(&size, sizeof(size)), IoParameters::READ_TIMEOUT())){
        if (size > IoParameters::MAX_DATA_BYTE_SIZE()){
          LOGE << "Recieved invalid packet size. Reject!";
          return {};
        }
        std::vector<uint8_t> message(size);

        if (sync_read_message(socket, boost::asio::buffer(message), IoParameters::READ_TIMEOUT())){
          proto_t recieved_data;
          if (recieved_data.ParseFromArray(message.data(), message.size()))
            return {recieved_data};
          else {
            LOGE << "Unable to deserialize proto message";
          }
        } else {
          LOGE << "Unable to fetch message";
        }
      } else {
        LOGE << "Unable to fetch message size.";
      }

      return {};
    }

    template<typename buffer_t, typename stream_t>
    static auto sync_send_message(stream_t &socket, buffer_t const &buffer){
      boost::system::error_code ecode;
      boost::asio::write(socket, boost::asio::buffer(buffer), ecode);

      if (ecode){
        LOGW << "Fail to write to socket" << ". Reason: "
             << ecode.message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename stream_t>
    static bool sync_send_proto(stream_t &socket, proto_t const& data){
      size_t size = data.ByteSizeLong();
      if (size >= IoParameters::MAX_DATA_BYTE_SIZE())
        LOGW << "Message size to big! Server maybe reject it.";

      if (!sync_send_message(socket, boost::asio::buffer(&size, sizeof(size)))){
        LOGW << "Unable to send proto struct size.";
        return false;
      }

      auto encoded = data.SerializeAsString();
      if (!sync_send_message(socket, boost::asio::buffer(encoded))){
        LOGW << "Unable to send serialized data.";
        return false;
      }

      return true;
    }

  };
};

#endif//SRP_IO_HPP
