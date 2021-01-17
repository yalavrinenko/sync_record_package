//
// Created by yalavrinenko on 17.01.2021.
//

#ifndef SRP_NOTIFIED_DEQUEU_HPP
#define SRP_NOTIFIED_DEQUEU_HPP
#include <list>
#include <mutex>
#include <condition_variable>

template<typename data_t>
class notified_deque {
public:
  void push(data_t const &v) {
    std::lock_guard lg(io_mutex_);
    container_.push_back(v);
    notify();
  }

  void push(data_t &&v) {
    std::lock_guard lg(io_mutex_);
    container_.emplace_back(std::move(v));
    notify();
  }

  bool empty() const {
    std::lock_guard lg(io_mutex_);
    return container_.empty();
  }

  size_t size() const {
    std::lock_guard lg(io_mutex_);
    return container_.size();
  }

  data_t &front() { return container_.front(); }

  data_t const &front() const { return container_.front(); }

  void pop() {
    std::lock_guard lg(io_mutex_);
    container_.erase(container_.begin());
  }

  bool wait_for_data(){
    if (empty() && !die_){
      std::unique_lock lock(io_mutex_);
      cv_.wait(lock);
    }
    return !empty();
  }

  void kill() {
    if (!die_) {
      die_ = true;
      notify();
    }
  }

  ~notified_deque() {
    kill();
  }

private:
  void notify() {
    cv_.notify_one();
  }

  std::list<data_t> container_;
  std::atomic<bool> die_{false};
  mutable std::mutex io_mutex_;
  std::condition_variable cv_;
};

#endif//SRP_NOTIFIED_DEQUEU_HPP
