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

  void after_load_init();

  void add_log_info(long device_key, const std::string& formatted_message);

  std::unique_ptr<srp::ui_control_client>& control();

  ~UiAppCore() override;

  Q_INVOKABLE void timerRecordEvent();
  Q_INVOKABLE void stopRecording();
  Q_INVOKABLE void startRecording();
  Q_INVOKABLE void stopControl();
  Q_INVOKABLE void timerSyncRecordEvent();
  Q_INVOKABLE void emitNextSyncPoint();
  Q_INVOKABLE void sendProbSignal();
  Q_INVOKABLE void startSyncRecording();
  Q_INVOKABLE void stopSyncRecording();
  Q_INVOKABLE void checkSignal();

  signals:
      void recordTimerEvent(QString elapsedTime);
      void setupSyncSenderTimer(long interval);
      void syncTimerEvent(QString elapsedTime);
      void showSyncPointId(unsigned long syncId);
      void stopTimers();

      void showCaptureLog(long key, QString message);

protected:
  std::optional<std::chrono::high_resolution_clock::time_point> main_record_start_{};
  std::optional<std::chrono::high_resolution_clock::time_point> sync_record_start_{};
  bool sync_mode_{false};
  bool is_recording_{false};
  std::unique_ptr<srp::ui_control_client> ui_control_;
  srp::UiControlOption options_;
  size_t sync_point_id_;
};

#endif//SRP_UIAPPCORE_HPP
