//
// Created by yalavrinenko on 21.01.2021.
//

#include "UiAppCore.hpp"

auto duration_to_str(auto const& duration){
  auto in_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

  char time_str[256];

  auto millis = in_millis % 1000; in_millis /= 1000;
  auto hh = (in_millis / (60 * 60)); in_millis %= 60 * 60;
  auto mm = in_millis / 60; in_millis %= 60;
  auto ss = in_millis;

  std::sprintf(time_str, "%02ld:%02ld:%02ld.%03ld", hh, mm, ss, millis);
  return std::string{time_str};
}

void UiAppCore::timerRecordEvent() {
  if (main_record_start_) {
    auto duration = std::chrono::high_resolution_clock::now() - *main_record_start_;
    emit recordTimerEvent(QString(duration_to_str(duration).c_str()));
  } else {
    main_record_start_ = std::chrono::high_resolution_clock::now();
    emit recordTimerEvent(QString("00:00:00"));
  }
}
void UiAppCore::stopRecording() {
  main_record_start_ = {};
}
void UiAppCore::startRecording() {

}
