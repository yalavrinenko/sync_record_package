//
// Created by yalavrinenko on 23.04.2020.
//

#ifndef DSCS_GUI_INPUT_HPP
#define DSCS_GUI_INPUT_HPP
#include "guilogger.hpp"
#include <memory>
#include <utility>
#include <vector>
namespace gui {

  class icontrol {
  public:
    using callback_t = std::function<void(icontrol &)>;
    using setter_t = callback_t;

    template<typename return_type>
    using getter_t = std::function<return_type(icontrol &)>;

    virtual void draw() = 0;
  };

  class gui_controls : public ilogger_entry {
  public:
    explicit gui_controls(std::shared_ptr<struct logger_window> factory, std::string name);

    template<typename CT, typename... T>
    void add_control(T &&... args) {
      controls_.emplace_back(std::make_unique<CT>(args...));
    }

    template<typename control_type>
    void add_control(std::unique_ptr<control_type> &&controls) {
      controls_.emplace_back(std::move(controls));
    }

    void flush() override;

    void draw_impl() override;

  protected:
    std::vector<std::unique_ptr<icontrol>> controls_;
  };

  class button_control : public icontrol {
  public:
    button_control() = default;

    button_control(std::string text, callback_t callback)
        : button_text_{std::move(text)}, callback_{std::move(callback)} {}

    void draw() override {
      if (!button_text_.empty() && ImGui::Button(button_text_.c_str())) { callback_(*this); }
    }

  protected:
    std::string button_text_;
    callback_t callback_;
  };

  template<typename value_type>
  class value_setter {
  public:
    using value_t = value_type;

    value_setter(icontrol::setter_t getter, icontrol::getter_t<value_t> setter) : value_setter_{std::move(getter)}, value_getter_{std::move(setter)} {}

    [[nodiscard]] value_t const &value() const { return value_; }
    value_t value() { return value_getter_; }

  protected:
    value_t value_;
    icontrol::setter_t value_setter_;
    icontrol::getter_t<value_t> value_getter_;
  };

  class slider_control : public icontrol, public value_setter<float> {
  public:
    slider_control(std::string text, callback_t callback, getter_t<double> setter = nullptr,
                   std::pair<double, double> range = {-360, 360})
        : value_setter<float>(std::move(callback), std::move(setter)), text_{std::move(text)}, range_{std::move(range)} {
    }
    void draw() override;
  protected:
    std::string text_;
    std::pair<double, double> range_;
  };

  class angle_control : public slider_control {
  public:
    angle_control(std::string text, callback_t callback, getter_t<double> setter = nullptr,
                  std::pair<double, double> range = {-360, 360})
        : slider_control(std::move(text), std::move(callback), std::move(setter), std::move(range)) {}

    void draw() override;

    [[nodiscard]] double angle() const { return this->value(); }

  protected:
  };

  class int_entry_control : public icontrol, public value_setter<int> {
  public:
    int_entry_control(std::string text, icontrol::setter_t getter, icontrol::getter_t<int> setter,
                      int min_value = 1);
    void draw() override;

  protected:
    std::string text_;
    int min_;
  };

  template<typename button_type, std::size_t N>
  class button_group : public icontrol {
  public:
    using button_t = button_type;

    button_group(std::initializer_list<button_t> &&list) {
      auto i = 0;
      for (auto &&e : list) { buttons_[i++] = std::move(e); }
    }
    void draw() override {
      ImGui::Columns(N, nullptr, false);
      for (auto &button : buttons_) {
        button.draw();
        ImGui::NextColumn();
      }
      ImGui::Columns(1);
    }

  protected:
    std::array<button_control, N> buttons_;
  };
}// namespace gui

#endif//DSCS_GUI_INPUT_HPP
