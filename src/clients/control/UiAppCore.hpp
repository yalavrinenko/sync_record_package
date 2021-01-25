//
// Created by yalavrinenko on 21.01.2021.
//

#ifndef SRP_UIAPPCORE_HPP
#define SRP_UIAPPCORE_HPP

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <chrono>
#include <memory>
#include <options.pb.h>

namespace srp{
  class ui_control_client;
}

class UiAppCore: public QObject{
Q_OBJECT
public:
  UiAppCore(std::unique_ptr<srp::ui_control_client> ui_control, srp::UiControlOption options, QObject *parent = nullptr);

  ~UiAppCore() override;

  Q_INVOKABLE void timerRecordEvent();
  Q_INVOKABLE void stopRecording();
  Q_INVOKABLE void startRecording();
  Q_INVOKABLE void stopControl();

  signals:
      void recordTimerEvent(QString elapsedTime);
      void setupSyncSenderTimer(long interval);

protected:
  std::optional<std::chrono::high_resolution_clock::time_point> main_record_start_{};
  std::unique_ptr<srp::ui_control_client> ui_control_;
  srp::UiControlOption options_;
};

#endif//SRP_UIAPPCORE_HPP
