//
// Created by yalavrinenko on 07.03.2021.
//

#include "pupil_eye_io.hpp"
#include <utils/logger.hpp>
#include <boost/format.hpp>

#include <zmq.hpp>

namespace srp{
  class pupil_eye_io::pupil_core_impl{
  public:
    pupil_core_impl(std::string host, long port): host_(std::move(host)), port_{port}{
      open_connection();
    }

    std::string version() {
      return communicate("v");
    }

    std::string start_recording(std::filesystem::path const& path){
      communicate("T 0");
      return communicate((boost::format("R %1%") % path.generic_string()).str());
    }
    std::chrono::milliseconds stop_recording() {
      communicate("r");
      return recording_time();
    }

    std::chrono::milliseconds recording_time(){
      auto time_str = communicate("t");
      return std::chrono::milliseconds(static_cast<unsigned long long>(std::stod(time_str) * 1000.0));
    }

    ~pupil_core_impl(){
      close_connection();
    }
  private:
    void open_connection();

    void close_connection();

    std::string communicate(std::string const& message);

    struct zmq_connection {
      zmq::context_t context;
      zmq::socket_t io_socket;
      zmq_connection(std::string const& host, long port): io_socket(context, zmq::socket_type::req){
        auto endpoint = boost::format ("tcp://%1%:%2%") % host % port;
        io_socket.connect(endpoint.str());
        if (!io_socket)
          LOGW << "Unable to connect io_socket to endpoint:" << endpoint;
      }
    };

    std::unique_ptr<zmq_connection> active_connection_ = nullptr;

    std::string host_;
    long port_;
  };

  void pupil_eye_io::pupil_core_impl::open_connection() {
    LOGD << boost::format("Open connection to Pupil Capture/Service at %1%:%2%") % host_ % port_;
    try {
      active_connection_ = std::make_unique<zmq_connection>(host_, port_);
    }
    catch (zmq::error_t const &e){
      LOGE << boost::format ("Fail to connect to Pupil Capture/Service at %1%:%2%. Reason (code %4%) %3%") % host_
                  % port_ % e.what() %e.num();
      throw e;
    }
  }
  void pupil_eye_io::pupil_core_impl::close_connection() {
    active_connection_.reset(nullptr);
  }

  std::string pupil_eye_io::pupil_core_impl::communicate(const std::string &message) {
    try {
      active_connection_->io_socket.send(zmq::buffer(message), zmq::send_flags::dontwait);

      zmq::message_t receive_message;
      auto rcvd = active_connection_->io_socket.recv(receive_message);
      if (!rcvd){
        LOGE << "Receive empty response on message: " << message;
        throw zmq::error_t();
      }

      return receive_message.to_string();
    } catch (zmq::error_t const &e){
      LOGE << boost::format ("Fail to send message [%1%] to remote server [%2%:%3%]. Reason (code %4%): %5%]")
                            % message % host_ % port_ % e.num() % e.what();
      throw e;
    };
    return {};
  }
}

srp::pupil_eye_io::~pupil_eye_io() = default;

srp::pupil_eye_io::pupil_eye_io(std::string host, long port): core_{std::make_unique<pupil_core_impl>(std::move(host), port)} {
}
std::string srp::pupil_eye_io::name() const {
  return "Pupil Capture Software. Version: " + core_->version();
}
std::chrono::milliseconds srp::pupil_eye_io::timestamp() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(core_->recording_time());
}
std::string srp::pupil_eye_io::start_recording(const std::filesystem::path &path) {
  return core_->start_recording(path);
}
std::chrono::milliseconds srp::pupil_eye_io::stop_recording() {
  return core_->stop_recording();
}
