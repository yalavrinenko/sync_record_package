//
// Created by yalavrinenko on 20.03.2020.
//

#ifndef DSCS_GUI_ENTRIES_HPP
#define DSCS_GUI_ENTRIES_HPP
#include "guilogger.hpp"
#include "moving_window.hpp"
#include <cmath>
#include <map>

namespace gui {
class text_entry : public ilogger_entry {
public:
  explicit text_entry(std::shared_ptr<class logger_window> f, std::string name)
      : ilogger_entry(std::move(f),std::move(name)) {}
  void log(std::string key, std::string const &value) {
    text_data_.tmp().emplace_back(std::move(key), value);
  }

  void flush() override {
    std::lock_guard<std::mutex> lg(io_mutex);
    text_data_.swap();
  }

protected:
  void draw_impl() override;

  cloned_data<std::pair<std::string, std::string>> text_data_;
};

class numeric_entry : public text_entry {
public:
  explicit numeric_entry(std::shared_ptr<class logger_window> f,
                         std::string name): text_entry(std::move(f), std::move(name)) {}

  void log(std::string key, double value, double max = 0) {
    if (max == 0)
      text_entry::log(key, std::to_string(value));
    else
      numeric_data_.tmp().emplace_back(std::move(key),
                                       std::make_pair(value, max));
  }

  void flush() override {
    text_entry::flush();
    std::lock_guard<std::mutex> lg(io_mutex);
    numeric_data_.swap();
  }

protected:
  void draw_impl() override;

  cloned_data<std::pair<std::string, std::pair<double, double>>>
      numeric_data_;
};

class moving_plot_entry: public ilogger_entry{
public:
  explicit moving_plot_entry(std::shared_ptr<class logger_window> f, std::string name, size_t N)
  : ilogger_entry(std::move(f), std::move(name)), N_{N}{ }

  void log(std::string key, double value){
    std::lock_guard l(io_mutex);
    if (!data_.count(key))
      data_.emplace(key, moving_window{N_});

    data_.at(key).add_data(value);
  }

  void flush() override {}

protected:
  void draw_impl() override;

  size_t N_;
  std::map<std::string, moving_window> data_;
};

class histo_plot_entry: public ilogger_entry{
public:
  using unary_function = std::function<double(int)>;

  explicit histo_plot_entry(std::shared_ptr<class logger_window> factory,
                            std::string name);
  void flush() override;

  //void log(std::string key, std::vector<double> data);
  void log(std::string key, unary_function data);
protected:
  void draw_impl() override;
  //cloned_data<std::pair<std::string, std::vector<double>>> data_;
  cloned_data<std::pair<std::string, unary_function>> data_;
};

class polar_entry: public ilogger_entry{
public:
  using unary_function = std::function<std::pair<double, double>(int)>;

  static inline double pi() { return std::atan(1.0) * 4.0; }

  explicit polar_entry(std::shared_ptr<class logger_window> factory,
                       std::string name, double max_r);
  void log(std::string key, unary_function const &function){
    data_.tmp().emplace_back(std::move(key), function);
  }
  void flush() override;
protected:
  void draw_impl() override;

  cloned_data<std::pair<std::string, unary_function>> data_;
  double const max_r_;
};

class radar_entry: public ilogger_entry{
public:

  struct radar_point{
    double r;
    double phi;
    std::string description;
  };

  using unary_function = std::function<std::pair<double, double>(int)>;

  static inline double pi() { return std::atan(1.0) * 4.0; }

  explicit radar_entry(std::shared_ptr<class logger_window> factory,
                       std::string name, double max_r, size_t segments);

  void log(radar_point point){
    data_.tmp().emplace_back(point);
  }

  void change_range(double max_r, size_t segments) {
    max_r_ = max_r; segments_ = segments;
  }

  void flush() override { data_.swap(); }

protected:
  void draw_impl() override;

  cloned_data<radar_point> data_;
  double max_r_;
  size_t segments_{1};
};
} // namespace gui
#endif // DSCS_GUI_ENTRIES_HPP
