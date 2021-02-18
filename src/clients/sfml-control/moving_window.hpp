//
// Created by yalavrinenko on 20.03.2020.
//

#ifndef DSCS_MOVING_WINDOW_HPP
#define DSCS_MOVING_WINDOW_HPP
#include <deque>

class moving_window: public std::deque<double>{
public:
  explicit moving_window(unsigned long window_size): window_size_{window_size}{
  }

  void add_data(double point){
    if (this->size() >= window_size_)
      this->pop_front();
    this->push_back(point);
  }

  unsigned long window_size() const {
    return window_size_;
  }

  void resize_window(unsigned long window_size){
    window_size_ = window_size;
    while (size() > window_size_)
      pop_front();
  }
protected:
  size_t window_size_;

private:
  using base_container = std::deque<double>;
  using base_container::push_back;
  using base_container::push_front;
  using base_container::pop_front;
  using base_container::pop_back;
  using base_container::emplace_back;
  using base_container::emplace;
  using base_container::insert;
  using base_container::resize;
};

#endif // DSCS_MOVING_WINDOW_HPP
