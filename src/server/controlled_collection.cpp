//
// Created by yalavrinenko on 19.01.2021.
//

#include "controlled_collection.hpp"
#include <server/rec_client.hpp>

std::atomic<size_t> srp::controlled_device_collection::client_id_counter_ {0};

std::shared_ptr<srp::controlled_device_collection> srp::controlled_device_collection::create_collection() {
  class builder : public controlled_device_collection{
  public:
    builder() = default;
  };

  return std::make_shared<builder>();
}

srp::session_builder srp::controlled_device_collection::create_client_builder(const std::shared_ptr<controlled_device_collection>& collection) {
  auto builder = [collection] (std::shared_ptr<base_session> raw_session) {
    auto client = srp::recording_client::from_base_session(std::move(raw_session));
    auto client_uid = srp::controlled_device_collection::client_id_counter_.load();
    ++srp::controlled_device_collection::client_id_counter_;

    client->init(client_uid);
    collection->add_capture_device(std::move(client));
  };

  return builder;
}
srp::session_builder srp::controlled_device_collection::create_monitor_builder(const std::shared_ptr<controlled_device_collection>& collection) {
  return [](auto raw) {
    LOGW << "Monitor client not impl.";
  };
}
