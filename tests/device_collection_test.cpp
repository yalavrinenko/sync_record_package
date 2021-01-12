//
// Created by yalavrinenko on 25.12.2020.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include "../src/capture_interface/capture.hpp"
#include "../src/server/device_collection.hpp"
#include "../src/utils/logger.hpp"
#include <boost/test/unit_test.hpp>

using namespace srp;

class dummy_client : public capture_i {
public:
  explicit dummy_client(size_t uid) { uid_ = uid; }
  void init(size_t uid) override { uid_ = uid; }
  std::optional<ClientCheckResponse> check() override { return std::optional<ClientResponse>(); }
  std::optional<ClientStartRecordResponse> start_recording(const std::string &path_template) override {
    LOGD << uid() << " start " << path_template;
    return std::optional<ClientResponse>(ClientResponse());
  }
  std::optional<ClientStopRecordResponse> stop_recording() override {
    LOGD << uid() << " stop ";
    return std::optional<ClientResponse>(ClientResponse());
  }
  std::optional<ClientSyncResponse> sync_time(size_t sync_point) override {
    LOGD << uid() << " sync " << sync_point;
    return std::optional<ClientResponse>(ClientResponse());
  }
};

class broken_client : public capture_i {
public:
  explicit broken_client(size_t uid) { uid_ = uid; }
  void init(size_t uid) override { uid_ = uid; }
  std::optional<ClientCheckResponse> check() override { return std::optional<ClientResponse>(); }
  std::optional<ClientStartRecordResponse> start_recording(const std::string &path_template) override {
    LOGD << uid() << " start " << path_template;
    return std::optional<ClientResponse>(ClientResponse());
  }
  std::optional<ClientStopRecordResponse> stop_recording() override {
    LOGD << uid() << " stop ";
    return std::optional<ClientResponse>(ClientResponse());
  }
  std::optional<ClientSyncResponse> sync_time(size_t sync_point) override {
    LOGD << uid() << " broken sync " << sync_point;
    return std::optional<ClientResponse>();
  }
};

BOOST_AUTO_TEST_SUITE(CollectionOperation);

BOOST_AUTO_TEST_CASE(CollectionCreateInserDelete) {
  device_collection devs;
  devs.add_capture_device(std::make_unique<dummy_client>(42));
  devs.add_capture_device(std::make_unique<dummy_client>(35));
  devs.add_capture_device(std::make_unique<dummy_client>(18));

  BOOST_REQUIRE(devs.device_count() == 3);
  devs.remove_capture_device(35);
  BOOST_REQUIRE(devs.device_count() == 2);
  devs.remove_capture_device(42);
  devs.remove_capture_device(18);
  BOOST_REQUIRE_THROW(devs.remove_capture_device(20), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(CollectionOperation) {
  struct protected_devs : public device_collection {
  public:
    void start(std::string const &s) { this->on_start_callback(s); }
    void stop() { this->on_stop_callback(); }
    void sync(size_t id) { this->on_sync_callback(id); }
  };

  protected_devs devs;
  devs.add_capture_device(std::make_unique<dummy_client>(42));
  devs.add_capture_device(std::make_unique<dummy_client>(35));
  devs.add_capture_device(std::make_unique<dummy_client>(18));

  devs.start("path");
  devs.sync(10);
  devs.sync(15);
  devs.sync(18);
  devs.stop();

  BOOST_REQUIRE(devs.device_count() == 3);
}

BOOST_AUTO_TEST_CASE(CollectionOperationBroken) {
  struct protected_devs : public device_collection {
  public:
    void start(std::string const &s) { this->on_start_callback(s); }
    void stop() { this->on_stop_callback(); }
    void sync(size_t id) { this->on_sync_callback(id); }
  };

  protected_devs devs;
  devs.add_capture_device(std::make_unique<dummy_client>(42));
  devs.add_capture_device(std::make_unique<broken_client>(35));
  devs.add_capture_device(std::make_unique<dummy_client>(18));

  devs.start("path");
  BOOST_REQUIRE(devs.device_count() == 3);
  devs.sync(10);
  BOOST_REQUIRE(devs.device_count() == 2);
  devs.sync(15);
  devs.sync(18);
  devs.stop();

  BOOST_REQUIRE(devs.device_count() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
