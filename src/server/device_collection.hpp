//
// Created by yalavrinenko on 25.12.2020.
//

#ifndef SRP_DEVICE_COLLECTION_HPP
#define SRP_DEVICE_COLLECTION_HPP
#include "../capture_interface/capture.hpp"
#include "../capture_interface/capture_control.hpp"
#include <future>
#include <functional>
#include <list>

namespace srp{
  using namespace std::chrono_literals;
  struct DeviceCollectionOpt{
    static std::chrono::milliseconds wait_timeout() {
      return std::chrono::milliseconds{1000};
    }
  };

  class device_collection {
  public:
    void add_capture_device(capture_device device);

    void remove_capture_device(size_t device_id);

    capture_device& find_device(size_t device_id);

    auto device_count() const { return slaves_.size(); }
  protected:
    void on_start_callback(std::string const &path_template);

    void on_stop_callback();

    void on_sync_callback(size_t sync_point);

    void process_result(capture_device &slave, std::optional<ClientResponse> const &response);

  private:
    using signal_sender_callback = std::function<std::pair<capture_device &, std::future<std::optional<ClientResponse>>>(capture_device&)>;

    void send_signal_implementation(signal_sender_callback const& signal_sender);

    void exclude_slave(auto &slave);

    capture_controller master_ = nullptr;
    std::list<capture_device> slaves_;
    std::list<capture_controller> monitors_;
    std::mutex slave_lock_;
  };
}
#endif//SRP_DEVICE_COLLECTION_HPP
