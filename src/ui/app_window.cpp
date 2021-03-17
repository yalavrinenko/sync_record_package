//
// Created by yalavrinenko on 17.03.2021.
//
#ifdef WIN32
#ifndef __kernel_entry
#define __kernel_entry
#endif
#endif

#include "app_window.hpp"
#include "ImGuiFileDialog.h"
#include <boost/format.hpp>
#include <boost/process.hpp>
#include <iostream>
#include <utils/logger.hpp>

namespace bp = boost::process;

class gui::app_window::app_instance {
public:
  app_instance(std::filesystem::path const &bin, std::filesystem::path const &json);

  bool is_running() const {
    return process_.running();
  }

  auto log_stdout() {
    std::lock_guard lock(io_mutex_);
    return output_.str();// += read_stream(out_stream_);
  }
  auto &log_stderr() {
    return error_;// += read_stream(err_stream_);
  }

  auto exit_code() const {
    if (!process_)
      return process_.exit_code();
    else
      return 0;
  }

  int close() {
    if (process_) { process_.terminate(); }
    process_.wait();
    read_thread_.wait();
    return process_.exit_code();
  }

  auto is_updated() {
    return output_updated_.exchange(false);
  }

  ~app_instance() {
    if (process_) close();
  }

private:
  bp::ipstream out_stream_;
  bp::ipstream err_stream_;
  mutable bp::child process_;

  std::future<void> read_thread_;
  std::mutex io_mutex_;

  std::stringstream output_;
  std::stringstream error_;

  std::atomic<bool> output_updated_{false};
};

gui::app_window::app_instance::app_instance(const std::filesystem::path &bin, const std::filesystem::path &json) {
  process_ = bp::child(bin.generic_string(), json.generic_string(), bp::std_out > out_stream_, bp::std_err > err_stream_);
  read_thread_ = std::async(std::launch::async, [this]() {
    std::string line;
    while (process_ && std::getline(out_stream_, line)) {
      std::lock_guard lock(io_mutex_);
      output_ << line << "\n";
      output_updated_.store(true);
    }
  });
}

void gui::app_window::draw() {
  ImGui::Separator();
  ImGui::TextWrapped("%s", app_->log_stdout().c_str());
  ImGui::Separator();

  std::string close_text = "Terminate App.";
  if (!app_ || !app_->is_running())
    close_text = "Close window";

  if (ImGui::Button(close_text.c_str(), {150, 30})) {
    app_->close();
    is_started_ = app_->is_running();
    if (on_close_){
      on_close_(linked_window_);
    }
  }

  ImGui::SameLine();
  if (app_ && app_->is_running()){
    ImGui::TextColored({0, 255, 0, 255}, "%s","Application is running...");
  } else {
    ImGui::TextColored({255, 0, 0, 255}, "%s",
                       ("Application finished with exit code " + std::to_string(app_->exit_code())).c_str());
  }

  if (last_scroll_position_ != ImGui::GetScrollMaxY()) {
    last_scroll_position_ = ImGui::GetScrollMaxY();
    ImGui::SetScrollY(last_scroll_position_);
  }
}
gui::app_window::app_window(std::filesystem::path bin, std::filesystem::path json, on_close_t on_close)
    : runner_{std::move(bin)}, json_{std::move(json)}, on_close_(std::move(on_close)) {
  try {
    app_ = std::make_unique<app_instance>(runner_, json_);
    if (app_)
      is_started_ = app_->is_running();
  } catch (std::exception &e) {
    LOGE << "Fail to start process " << runner_ << " with args: " << json_ << ". Reason:" << e.what();
  }
};

gui::app_window::~app_window() {
  if (app_) app_->close();
};
