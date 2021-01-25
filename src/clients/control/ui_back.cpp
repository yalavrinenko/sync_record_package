//
// Created by yalavrinenko on 20.01.2021.
//

#include "ui_back.hpp"
#include <utils/logger.hpp>
#include "ui_control_client.hpp"
#include <utils/io.hpp>
#include <session.pb.h>
#include <options.pb.h>
#include "UiAppCore.hpp"

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

  QApplication app(argc, argv);
  QQmlApplicationEngine engine;

  UiAppCore core(std::move(controller), options.value());
  QQmlContext *context = engine.rootContext();
  context->setContextProperty("baseConnections", &core);

  engine.load(QUrl(QStringLiteral("/home/yalavrinenko/Files/git/sync_record_package/src/clients/control/ui.qml")));
  return app.exec();
}
