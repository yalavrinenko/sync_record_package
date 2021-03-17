//
// Created by yalavrinenko on 17.03.2021.
//

#include "app_runner.hpp"
#include "ImGuiFileDialog.h"
#include <boost/format.hpp>
#include <utils/logger.hpp>

struct ConfigSelector {
  static auto &default_path() {
    static std::filesystem::path path{"."};
    return path;
  }
};

void srp::app_runner::draw() {
  using namespace std::string_literals;

  for (auto const &bt : buttons_) {
    if (ImGui::Button(bt.data(), {300, 0})) {
      ImGuiFileDialog ::Instance()->OpenDialog("Select"s + bt.data(), "Choose Config File", ".json",
                                               ConfigSelector::default_path().generic_string()+"/");
    }
  }

  for (std::weakly_incrementable auto i : std::views::iota(0, 3)) {
    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGuiFileDialog::Instance()->Display("Select"s + buttons_[i].data(), flags, {300, 500})) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        auto json_path = ImGuiFileDialog ::Instance()->GetFilePathName();
        ConfigSelector::default_path() = ImGuiFileDialog ::Instance()->GetCurrentPath();

        auto on_close = [this](auto *sender){
          LOGD << "Remove window " << sender;
          window_->remove_logger(sender);
        };

        auto app = std::make_shared<gui::app_window>(runner_set_[i], json_path, on_close);
        if (app && app->is_running()) {
          auto name = boost::format("%1%: %2% %3%") % (window_uid_++) % runner_set_[i] % json_path;
          auto logger = window_ ->create_logger<gui::gui_controls>(name.str());
          app->link_window(logger.get());
          logger->set_size({800, 400});
          logger->set_flags(ImGuiWindowFlags_NoResize);
          logger->add_control(std::move(app));
        }
      }
      ImGuiFileDialog ::Instance()->Close();
    }
  }
}
