//
// Created by yalavrinenko on 23.04.2020.
//
#include "gui_input.hpp"

#include <utility>
gui::gui_controls::gui_controls(std::shared_ptr<struct logger_window> factory, std::string name) : ilogger_entry(
    std::move(factory), std::move(name)) {
}

void gui::gui_controls::flush() { }

void gui::gui_controls::draw_impl() {
  for (auto const &control_ptr : controls_){
    control_ptr->draw();
  }
}

void gui::angle_control::draw() {
  if (value_getter_)
    value_ = value_getter_(*this);

  if (ImGui::SliderAngle(text_.c_str(), &value_, range_.first, range_.second)){ value_setter_(*this);
  }
}
void gui::slider_control::draw() {
  if (value_getter_)
    value_ = value_getter_(*this);

  if (ImGui::SliderFloat(text_.c_str(), &value_, range_.first, range_.second)){
    value_setter_(*this);
  }
}

void gui::int_entry_control::draw() {
  if (value_getter_)
    value_ = value_getter_(*this);
  if (ImGui::InputInt(text_.c_str(), &value_, min_)){
    value_setter_(*this);
  }
}

gui::int_entry_control::int_entry_control(std::string text, gui::icontrol::setter_t getter,
                                          gui::icontrol::getter_t<int> setter, int min_value)
    : value_setter(std::move(getter), std::move(setter)), text_(std::move(text)), min_(min_value){}
