//
// Created by yalavrinenko on 12.01.2021.
//

#include <clients/netclient.hpp>
#include <future>
#include <clients/audio/audio_instance.hpp>
#include <fstream>
#include <utils/logger.hpp>
#include <algorithm>
#include <iterator>
#include <google/protobuf/util/json_util.h>

int main(int argc, char** argv){
  if (argc < 2){
    LOGE << "No input files. Use ./capture_client optionpath.json";
    std::exit(1);
  }

  std::ifstream in(argv[1]);

  std::string json;
  std::string line;
  while (std::getline(in, line)){
    json += line + "\n";
  }
  LOGD << "Read config from " << argv[1] << "\n" << json;

  srp::OptionEntry entry;
  auto status = google::protobuf::util::JsonStringToMessage(json, &entry);

  if (!status.ok()) {
    LOGE << "Fail to parse input json. Reason: " << status.message();
    std::exit(1);
  }

  if (entry.tag() == "audio") {
    srp::AudioCaptureOptions options;
    entry.options().UnpackTo(&options);

    srp::audio_instance ai(options);
    auto check = ai.check();
    if (check) { LOGD << check->check_ok() << "\n" << check->info(); }

    using namespace std::chrono_literals;

    volatile bool make_snap = true;
    auto monitor = std::async(std::launch::async, [&ai, &make_snap](){
      while (make_snap){
        auto state = ai.state();
        LOGD << "Monitor: " << state->state() << " " << state->frames() << " " << state->duration_sec() << " " << state->average_fps();
        std::this_thread::sleep_for(100ms);
      }
    });

    auto start = ai.start_recording("demo_1");
    if (start){
      LOGD << "Start rec. to " << *start->data_path().begin() << " " << *start->sync_point_path().begin();
    }

    std::this_thread::sleep_for(300s);

    for (auto i = 0; i < 5; ++i){
      auto sync = ai.sync_time(i);
      LOGD << sync->frames() << " " << sync->duration_sec() << " " << sync->average_fps();
      std::this_thread::sleep_for(1s);
    }

    auto stop = ai.stop_recording();
    if (stop){
      LOGD << "Rec. " << stop->frames() << " " << stop->duration_sec() << " " << stop->average_fps();
    }

    std::this_thread::sleep_for(10s);
    make_snap = false;
  }
  return 0;
}