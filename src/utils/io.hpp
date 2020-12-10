//
// Created by yalavrinenko on 10.12.2020.
//

#ifndef SRP_IO_HPP
#define SRP_IO_HPP

#include "logger.hpp"
#include <boost/asio.hpp>

namespace srp {
  struct NetUtils {
    template<typename buffer_t, typename stream_t>
    static auto sync_read_message(stream_t &socket, buffer_t const &buffer){
      boost::system::error_code ecode;
      boost::asio::read(socket, boost::asio::buffer(buffer), ecode);

      if (ecode){
        LOGW << "Unable to read connection type from " << socket.remote_endpoint().address().to_string() << ". Reason: "
             << ecode.message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename stream_t>
    static std::optional<proto_t> sync_read_proto(stream_t &socket){
      size_t size;
      if (sync_read_message(socket, boost::asio::buffer(&size, sizeof(size)))){
        std::vector<uint8_t> message(size);

        if (sync_read_message(socket, boost::asio::buffer(message))){
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
  };
};

#endif//SRP_IO_HPP
