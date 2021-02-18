//
// Created by yalavrinenko on 12.01.2021.
//

#include "audio/audio_instance.hpp"
#include "bitalino/bitalino_instance.hpp"
#include "netclient.hpp"
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <utils/logger.hpp>
#include <utils/io.hpp>

class InstanceBuilder {
public:
  static auto build(srp::OptionEntry const &entry);
private:
  template<typename proto_t, typename instance_t>
  static auto builder_entry() {
    return [](auto const &entry) {
      proto_t option;
      entry.options().UnpackTo(&option);
      return std::make_unique<instance_t>(option);
    };
  }
  static auto &builder_map() {
    using namespace std::string_literals;
    using build_function = std::function<std::unique_ptr<srp::capture_i>(srp::OptionEntry const &)>;
    static std::map<std::string, build_function> builders{
        {"audio"s, InstanceBuilder::builder_entry<srp::AudioCaptureOptions, srp::audio_instance>()},
        {"bitalino"s, InstanceBuilder::builder_entry<srp::BitalinoCaptureOptions, srp::bitalino_instance>()},
    };
    return builders;
  }
};

auto InstanceBuilder::build(const srp::OptionEntry &entry) { return std::invoke(builder_map()[entry.tag()], std::cref(entry)); }

auto create_client_instance(std::string const &json_options) {
  auto entry = srp::ProtoUtils::message_from_json<srp::OptionEntry>(json_options);

  if (!entry) {
    return std::pair{std::unique_ptr<srp::capture_i>(nullptr), srp::ControlServerOption{}};
  }

  return std::pair{InstanceBuilder::build(entry.value()), entry.value().control_server()};
}

int main(int argc, char **argv) {
  if (argc < 2) {
    LOGE << "No input files. Use ./capture_client option_path.json";
    std::exit(1);
  }

  srp::netclient client;

  for (auto client_entry_id = 1; client_entry_id < argc; ++client_entry_id) {
    auto json = srp::IoUtils::read_json(argv[client_entry_id]);
    LOGD << "Read config from json: \n" << json;

    auto [instance, serv_options] = create_client_instance(json);
    if (instance == nullptr){
      LOGE << "Fail to create capture instance.";
    } else {
      client.add_client_instance(std::move(instance), serv_options);
    }

    //wait for free device
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);
  }

  LOGD << "Init " << client.instances_count() << " capture instance.";
  client.run();

  return 0;
}