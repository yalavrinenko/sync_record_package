//
// Created by yalavrinenko on 21.01.2021.
//

#ifndef SRP_UIAPPCORE_HPP
#define SRP_UIAPPCORE_HPP

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <chrono>

class UiAppCore: public QObject{
Q_OBJECT
public:
  UiAppCore(QObject* parent = nullptr): QObject(parent) {
  }

  Q_INVOKABLE void timerRecordEvent();
  Q_INVOKABLE void stopRecording();
  Q_INVOKABLE void startRecording();

  signals:
      void recordTimerEvent(QString elapsedTime);

protected:
  std::optional<std::chrono::high_resolution_clock::time_point> main_record_start_{};
};

#endif//SRP_UIAPPCORE_HPP
