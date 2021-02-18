//
// Created by yalavrinenko on 19.03.2020.
//

#ifndef DSCS_GUILOGGER_HPP
#define DSCS_GUILOGGER_HPP

#include <memory>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
#include <future>
#include <imgui.h>
#include <mutex>
#include <vector>

namespace gui {
class ilogger_entry{
public:
  explicit ilogger_entry(std::shared_ptr<class logger_window> factory, std::string name);
  virtual void draw();
  virtual void flush() = 0;

  std::shared_ptr<class logger_window>& factory() { return linked_factory_; }

  virtual ~ilogger_entry() = default;
protected:
  virtual void draw_impl() = 0;
  std::shared_ptr<class logger_window> linked_factory_;
  std::string name_;
  std::mutex io_mutex;
};

class logger_window: public std::enable_shared_from_this<logger_window> {
public:
  static std::shared_ptr<logger_window> create(std::string name="") {
    return std::shared_ptr<logger_window>(new logger_window(std::move(name)));
  }

  void draw();

  bool is_open() const { return window_.isOpen(); }

  template <typename LoggerType, typename ... TInitArgs>
  std::shared_ptr<LoggerType> create_logger(TInitArgs&& ... args){
    auto ptr = std::make_shared<LoggerType>(shared_from_this(), std::forward<TInitArgs>(args)...);
    entries_.emplace_back(ptr);
    return ptr;
  }

  void flush(){
    for (auto &e : entries_)
      e->flush();
  }

  ~logger_window();
protected:
  explicit logger_window(std::string name)
      : window_title_{std::move(name)},
        window_(sf::VideoMode(1800, 1000), window_title_) {
    init_window();
  }

  void init_window();

  void events();

private:
  std::string window_title_;
  sf::RenderWindow window_;
  sf::Clock delta_clock_;
  ImGuiContext* ctx_{nullptr};
  std::vector<std::shared_ptr<ilogger_entry>> entries_;
};

using plog_window = std::shared_ptr<logger_window>;

class logger_environment : public std::enable_shared_from_this<logger_environment> {
public:
  static std::shared_ptr<logger_environment> create() {
    static auto factory = std::shared_ptr<logger_environment>(new logger_environment());
    return factory;
  }

  std::shared_ptr<logger_window> create_logger(std::string name) {
    windows_.emplace_back(logger_window::create(std::move(name)));
    return windows_.back();
  }

  void flush();

  void draw();

  void stop() {
    //is_stop_sig_ = true;
  }

  ~logger_environment() {
    //wthread_.get();
  }
protected:
  explicit logger_environment(){
//    wthread_ = std::async(std::launch::async, [this](){ this-> draw(); });
  }

private:
  std::vector<std::shared_ptr<logger_window>> windows_;

  std::future<void> wthread_;

  bool is_stop_sig_ {false};
};

template <typename DataType, typename container=std::vector<DataType>>
class cloned_data{
public:
  void swap() {
    std::swap(main_, tmp_);
    tmp_.clear();
  }
  container& main() { return main_; }
  container& tmp() { return tmp_;}
private:
  container main_, tmp_;
};
} // namespace gui
#endif // DSCS_GUILOGGER_HPP
