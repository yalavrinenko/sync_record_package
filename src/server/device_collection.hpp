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

    [[nodiscard]] auto device_count() const { return slaves_.size(); }

    void add_capture_monitor(capture_controller monitor);
    void add_master_controller(capture_controller master);

  protected:
    void on_start_callback(std::string const &path_template);

    void on_stop_callback();

    void on_sync_callback(size_t sync_point);

    void on_state_callback();

    void on_check_callback();

    void process_result(capture_device &slave, std::optional<ClientResponse> const &response);

    srp::controller_callbacks controller_callback_set();

  private:
    using signal_sender_callback = std::function<std::pair<capture_device &, std::future<std::optional<ClientResponse>>>(capture_device&)>;

    void signal_broadcast(signal_sender_callback const& signal_sender);

    void signal_to_device(capture_device& device, signal_sender_callback const& signal_sender);

    void exclude_slave(auto &slave);

    capture_controller master_ = nullptr;

    std::list<capture_device> slaves_;

    std::list<capture_controller> monitors_;
    std::mutex slave_lock_;
  };
}
#endif//SRP_DEVICE_COLLECTION_HPP
