//
// Created by yalavrinenko on 17.03.2021.
//

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

  bool is_running() const { return process_; }

  auto log_stdout() {
    std::lock_guard lock(io_mutex_);
    return output_.str();// += read_stream(out_stream_);
  }
  auto &log_stderr() {
    return error_;// += read_stream(err_stream_);
  }

  int close() {
    if (process_) { process_.terminate(); }
    process_.wait();
    read_thread_.wait();
    return process_.exit_code();
  }

  ~app_instance() {
    if (process_)
      close();
  }

private:
  std::string read_stream(auto &s) {
    std::string line;
    std::getline(s, line);
    return line;
  }

  bp::ipstream out_stream_;
  bp::ipstream err_stream_;
  bp::child process_;

  std::future<void> read_thread_;
  std::mutex io_mutex_;

  std::stringstream output_;
  std::stringstream error_;
};

gui::app_window::app_instance::app_instance(const std::filesystem::path &bin, const std::filesystem::path &json){
  process_ = bp::child(bin.generic_string(), json.generic_string(), bp::std_out > out_stream_, bp::std_err > err_stream_);
  read_thread_ = std::async(std::launch::async, [this]() {
    std::string line;
    while (process_ && std::getline(out_stream_, line)){
      std::lock_guard lock(io_mutex_);
      output_ << line << "\n";
    }
  });
}



void gui::app_window::draw() {
  if (!is_started_) {
    if (ImGui::Button("Start", {100, 30}))
      ImGuiFileDialog ::Instance()->OpenDialog("SelectJson", "Choose Config File", ".json", ConfigSelector::default_path().generic_string());

    if (ImGuiFileDialog::Instance()->Display("SelectJson", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize, {300, 500})) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        auto json_path = ImGuiFileDialog ::Instance()->GetFilePathName();
        ConfigSelector::default_path() = ImGuiFileDialog ::Instance()->GetCurrentPath();

        try {
          app_ = std::make_unique<app_instance>(runner_, json_path);
          if (app_) is_started_ = app_->is_running();
        } catch (std::exception &e) { LOGE << "Fail to start process " << runner_ << " with args: " << json_path << ". Reason:" << e.what(); }
      }
      ImGuiFileDialog ::Instance()->Close();
    }
  } else {
    if (ImGui::Button("Close", {100, 30})) {
      app_->close();
      is_started_ = app_->is_running();
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", app_->log_stdout().c_str());
    ImGui::Separator();
  }
}
gui::app_window::app_window(std::filesystem::path bin) : runner_{std::move(bin)} {};
gui::app_window::~app_window() {
    if (app_)
      app_->close();
};
