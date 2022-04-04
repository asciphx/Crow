#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <chrono>
#include <thread>
#include <fstream>
#include <iterator>
#include "config.h"
#include "cc/logging.h"
namespace boost {
  namespace posix_time {
	class BOOST_SYMBOL_VISIBLE millseconds : public time_duration {
	public:template <typename T>
	  BOOST_CXX14_CONSTEXPR explicit millseconds(T const& s,
		typename boost::enable_if<boost::is_integral<T>, void>::type* = BOOST_DATE_TIME_NULLPTR) :
	  time_duration(0, 0, 0, numeric_cast<fractional_seconds_type>(s)) {}
	};
  }
}
namespace cc {
  namespace detail {
	static std::string directory_ = STATIC_DIRECTORY;
	/// Fast timer queue for fixed tick value.
	class dumb_timer_queue {
	public:
	  static int tick;
	  using key = std::pair<dumb_timer_queue*, int>;
	  void cancel(key& k) {
		auto self = k.first; k.first = nullptr;
		if (!self) return;
		unsigned int index = static_cast<unsigned>(k.second - self->step_);
		if (index < self->dq_.size())
		  self->dq_[index].second = nullptr;
	  }
	  /// Add a function to the queue.
	  key add(std::function<void()> f) {
		dq_.emplace_back(std::chrono::steady_clock::now(), std::move(f));
		int ret = step_ + dq_.size() - 1;
		LOG_DEBUG << "timer add inside: " << this << ' ' << ret;
		return { this, ret };
	  }
	  /// Process the queue: take functions out in time intervals and execute them.
	  void process() {
		if (!io_service_)return;
		auto now = std::chrono::steady_clock::now();
		while (!dq_.empty()) {
		  auto& x = dq_.front(); if (!x.second) goto _;
		  if (now - x.first < std::chrono::seconds(tick)) break;
		  x.second(); _: dq_.pop_front(); ++step_;
		}
	  }
	  void set_io_service(boost::asio::io_service& io_service) {
		io_service_ = &io_service;
	  }
	  dumb_timer_queue() noexcept {}
	private:
	  boost::asio::io_service* io_service_{};
	  std::deque<std::pair<decltype(std::chrono::steady_clock::now()), std::function<void()>>> dq_;
	  int step_{};
	};
  }
}
