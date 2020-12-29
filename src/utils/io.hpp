//
// Created by yalavrinenko on 10.12.2020.
//

#ifndef SRP_IO_HPP
#define SRP_IO_HPP

#include "logger.hpp"
#include <boost/asio.hpp>
#include <google/protobuf/util/delimited_message_util.h>

namespace srp {
  class message_parse_error : public std::exception {};

  struct NetUtils {

    struct IoParameters {
      static constexpr auto AWAIT_TIMEOUT() {
        return std::chrono::milliseconds(500);
      }
    };

    template<typename buffer_t, typename stream_t>
    static auto sync_read_message(stream_t &stream, buffer_t const &buffer) {
      if (stream >> buffer) {
        LOGW << "Unable to read from socket"
             << ". Reason: " << stream.error().message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename communication_t>
    static std::optional<proto_t> sync_read_proto(communication_t &comm) {
      proto_t message;
      if (comm->is_alive()){
        auto reader = [&]() {
          return comm->receive_message(&message);
        };

        auto waiter = std::async(std::launch::async, reader);

        auto status = waiter.wait_for(IoParameters::AWAIT_TIMEOUT());
        if (status == std::future_status::ready){
          return waiter.get() ? message : std::optional<proto_t>{};
        } else {
          comm->terminate();
          waiter.get();
          return {};
        }

      } else {
        LOGW << "Unable to read proto. Communication !is_alive.";
      }

      return {};
    }

    template<typename buffer_t, typename stream_t>
    static auto sync_send_message(stream_t &stream, buffer_t const &buffer) {
      stream << buffer; stream.flush();
      if (!stream) {
        LOGW << "Fail to write to socket"
             << ". Reason: " << stream.error().message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename communication_t>
    static bool sync_send_proto(communication_t &comm, proto_t const &data) {
      if (comm->is_alive()){
        return comm->send_message(&data);
      } else {
        LOGW << "Unable to send proto. Communication !is_alive";
      }
      return false;
    }
  };

  struct ProtoUtils {
    template<typename message_t>
    static message_t message_from_bytes(auto const &data) {
      message_t m;
      if (m.ParseFromString(data)) return m;
      else
        throw message_parse_error();
    }
  };
};// namespace srp

#endif//SRP_IO_HPP
