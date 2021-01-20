//
// Created by yalavrinenko on 20.01.2021.
//

#include "ui_back.hpp"
#include <utils/logger.hpp>
#include "ui_control_client.hpp"
#include <utils/io.hpp>
#include <session.pb.h>
#include <options.pb.h>

//#include <QApplication>
//#include <QQmlApplicationEngine>
namespace {
  template<typename message_t>
  std::pair<message_t, size_t> extract_response(auto const& ref){
    decltype(auto) resp_ref = dynamic_cast<srp::ClientResponse const&>(ref);
    return {srp::ProtoUtils::message_from_bytes<message_t>(resp_ref.data()), resp_ref.uid()};
  }
}
int srp::ui_back::start_ui(int argc, char **argv) {
  if (argc < 2) {
    LOGE << "Option file is required. Use ./app option.json.";
    return -1;
  }
  auto json = srp::IoUtils::read_json(argv[1]);
  auto options = srp::ProtoUtils::message_from_json<UiControlOption>(json);

  if (!options){
    return -1;
  }

  auto controller = std::make_unique<srp::ui_control_client>(options.value());

  srp::controller_callbacks callbacks;
  callbacks.add_callback(check_device, [](auto const &message){
    auto [check, who] = extract_response<ClientCheckResponse>(message);
    ClientResponse clr;
    std::string info = check.info();
    LOGD << "Receive CHECK response from " << who << " with value: " << check.check_ok() << " " << check.info();
  });

  controller->start(callbacks);
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1000s);
//  QApplication app(argc, argv);
//  QQmlApplicationEngine engine;
//  engine.load(QUrl(QStringLiteral("qrc:⁄main.qml")));
//  return app.exec();
  return 0;
}
