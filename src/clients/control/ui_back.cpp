//
// Created by yalavrinenko on 20.01.2021.
//

#include "ui_back.hpp"
#include "UiAppCore.hpp"
#include "ui_control_client.hpp"
#include <boost/format.hpp>
#include <options.pb.h>
#include <session.pb.h>
#include <utils/io.hpp>
#include <utils/logger.hpp>

namespace {
  template<typename message_t>
  std::pair<message_t, size_t> extract_response(auto const &ref) {
    decltype(auto) resp_ref = dynamic_cast<srp::ClientResponse const &>(ref);
    return {srp::ProtoUtils::message_from_bytes<message_t>(resp_ref.data()), resp_ref.uid()};
  }
}// namespace

int srp::ui_back::start_ui(int argc, char **argv) {
  if (argc < 2) {
    LOGE << "Option file is required. Use ./app option.json.";
    return -1;
  }
  auto json = srp::IoUtils::read_json(argv[1]);
  auto options = srp::ProtoUtils::message_from_json<UiControlOption>(json);

  if (!options) { return -1; }

  auto controller = std::make_unique<srp::ui_control_client>(options.value());

  QApplication app(argc, argv);
  QQmlApplicationEngine engine;

  UiAppCore core(std::move(controller), options.value());
  QQmlContext *context = engine.rootContext();
  context->setContextProperty("baseConnections", &core);

  engine.load(QUrl("ui.qml"));

  core.after_load_init();

  srp::controller_callbacks callbacks;

  auto callback_wrapper = [&core](auto function) {
    return [function, &core](auto const &message) {
      auto [formatted_string, who] = function(message);

      LOGD << formatted_string;
      core.add_log_info(who, formatted_string.str());
      return srp::ClientResponse();
    };
  };

  callbacks.add_callback(check_device, callback_wrapper([](auto const &message) {
                           auto [check, who] = extract_response<ClientCheckResponse>(message);
                           auto info_str = check.info();
                           std::replace(info_str.begin(), info_str.end(), '\n', '\t');
                           auto formatted_string = boost::format("CHECK request finished with status [%1%]. Addition message: [%2%].") %
                                                   ((check.check_ok()) ? "OK" : "FAIL") % info_str;
                           return std::pair{formatted_string, who};
                         }));

  callbacks.add_callback(start, callback_wrapper([](auto const &message) {
                           auto [data, who] = extract_response<ClientStartRecordResponse>(message);

                           std::string data_path, split_path;
                           for (auto i = 0; i < data.data_path_size(); ++i) {
                             data_path += data.data_path(i) + ";";
                             split_path += data.sync_point_path(i) + ";";
                           }

                           auto formatted_string =
                               boost::format("Start capturing data to [%1%] path. Split data will store into [%2%]") % data_path % split_path;
                           return std::pair{formatted_string, who};
                         }));

  callbacks.add_callback(stop, callback_wrapper([](auto const &message) {
                           auto [data, who] = extract_response<ClientStopRecordResponse>(message);
                           auto fstr = boost::format("Stop recording. Captured [%1%] frames in [%2%] seconds with average FPS [%3%]");
                           fstr = fstr % data.frames() % data.duration_sec() % data.average_fps();
                           return std::pair{fstr, who};
                         }));

  callbacks.add_callback(sync_time, callback_wrapper([](auto const &message) {
                           auto [data, who] = extract_response<ClientSyncResponse>(message);
                           auto fstr = boost::format("Set synchro. point [%1%] at [%2%] frame ([%3%] sec. from start)");
                           fstr = fstr % data.sync_point() % data.frames() % data.duration_sec();
                           return std::pair{fstr, who};
                         }));

  callbacks.add_callback(time, callback_wrapper([](auto const &message) {
                           auto [data, who] = extract_response<ClientTimeResponse>(message);

                           auto state_str = srp::ClientTimeResponse_CaptureState_Name(data.state());

                           auto fstr = boost::format("[%1%]:\tCurrent frame: [%2%]\tTime: [%3%]\tFPS: [%4%]");
                           fstr = fstr % state_str % data.frames() % data.duration_sec() % data.average_fps();
                           return std::pair{fstr, who};
                         }));

  callbacks.add_callback(disconnect, [&core](auto const &message) {
    return srp::ClientResponse();
  });


  core.control()->start(callbacks);


  return app.exec();
}
