//
// Created by yalavrinenko on 17.03.2021.
//

#ifndef SRP_APP_WINDOW_HPP
#define SRP_APP_WINDOW_HPP

#include <clients/sfml-control/gui_input.hpp>
#include <filesystem>
#include <boost/asio.hpp>

namespace gui {
  struct ConfigSelector{
    static auto& default_path() {
      static std::filesystem::path path{"."};
      return path;
    }
  };

  class app_window: public icontrol {
  public:
    explicit app_window(std::filesystem::path bin);

    void draw() override;

    ~app_window();
  private:
    class app_instance;

    std::unique_ptr<app_instance> app_;

    std::filesystem::path runner_;

    bool is_started_ = false;
  };
}


#endif//SRP_APP_WINDOW_HPP
