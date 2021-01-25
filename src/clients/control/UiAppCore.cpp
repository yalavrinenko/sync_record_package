//
// Created by yalavrinenko on 21.01.2021.
//

#include "UiAppCore.hpp"
#include "ui_control_client.hpp"
#include <QQuickWindow>
#include <protocols/actions.hpp>
#include <utility>

auto duration_to_str(auto const &duration) {
  auto in_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  char time_str[256];

  auto millis = in_millis % 1000;
  in_millis /= 1000;
  auto hh = (in_millis / (60 * 60));
  in_millis %= 60 * 60;
  auto mm = in_millis / 60;
  in_millis %= 60;
  auto ss = in_millis;

  std::sprintf(time_str, "%02ld:%02ld:%02ld.%03ld", hh, mm, ss, millis);
  return std::string{time_str};
}

UiAppCore::UiAppCore(std::unique_ptr<srp::ui_control_client> ui_control, srp::UiControlOption options, QObject *parent)
    : QObject(parent), ui_control_{std::move(ui_control)}, options_{std::move(options)} {
}
UiAppCore::~UiAppCore() = default;
void UiAppCore::stopControl() {
  if (is_recording_)
    stopRecording();

  if (ui_control_) ui_control_->stop();
}

void UiAppCore::timerRecordEvent() {
  if (main_record_start_) {
    auto duration = std::chrono::high_resolution_clock::now() - *main_record_start_;
    emit recordTimerEvent(QString(duration_to_str(duration).c_str()));
  }
}
void UiAppCore::stopRecording() {
  if (sync_mode_)
    stopSyncRecording();
  emit stopTimers();

  ui_control_->send_message(srp::ActionMessageBuilder::stop_message());
  main_record_start_ = {};
  is_recording_ = false;
}
void UiAppCore::startRecording() {
  main_record_start_ = std::chrono::high_resolution_clock::now();
  ui_control_->send_message(srp::ActionMessageBuilder::start_message(options_.record_path_template()));

  emit recordTimerEvent(QString("00:00:00"));
  is_recording_ = true;
}

void UiAppCore::timerSyncRecordEvent() {
  if (sync_record_start_) {
    auto duration = std::chrono::high_resolution_clock::now() - *sync_record_start_;
    emit syncTimerEvent(QString(duration_to_str(duration).c_str()));
  }
}
void UiAppCore::emitNextSyncPoint() {
  if (ui_control_) {
    ui_control_->send_message(srp::ActionMessageBuilder::sync_message(sync_point_id_));
    emit showSyncPointId(sync_point_id_);
    ++sync_point_id_;

    sync_record_start_ = std::chrono::high_resolution_clock::now();
  }
}
void UiAppCore::startSyncRecording() {
  sync_mode_ = true;
  sync_record_start_ = std::chrono::high_resolution_clock::now();
  sync_point_id_ = 0;
  emitNextSyncPoint();
}
void UiAppCore::stopSyncRecording() {
  sync_mode_ = false;
  sync_record_start_ = {};
  sync_point_id_ = 0;
  emitNextSyncPoint();
}

void UiAppCore::sendProbSignal() {
  if (ui_control_) ui_control_->send_message(srp::ActionMessageBuilder::state_request_message());
}
void UiAppCore::after_load_init() {
  emit setupSyncSenderTimer(options_.sync_point_interval_millis());
}
