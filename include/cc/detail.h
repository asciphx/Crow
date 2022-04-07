
#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>
#include <fstream>
#include <iterator>
#include "config.h"
#include "cc/logging.h"
namespace boost { namespace posix_time { class BOOST_SYMBOL_VISIBLE millseconds : public time_duration { public:template <typename T> BOOST_CXX14_CONSTEXPR explicit millseconds(T const& s, typename boost::enable_if<boost::is_integral<T>, void>::type* = BOOST_DATE_TIME_NULLPTR) : time_duration(0, 0, 0, numeric_cast<fractional_seconds_type>(s)) {} }; }}
namespace cc { static std::unordered_map<uint64_t, std::string> RES_CACHE_MENU = {}; static std::unordered_map<uint64_t, int64_t> RES_CACHE_TIME = {}; static tm* RES_NOW; static auto RES_last = std::chrono::steady_clock::now(); tm now() { if (std::chrono::steady_clock::now() - RES_last < std::chrono::seconds(1)) return *RES_NOW; time_t rawtime; std::time(&rawtime);
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(RES_NOW, &rawtime);
#else
 localtime_r(&rawtime, RES_NOW);
#endif
 RES_last = std::chrono::steady_clock::now(); return *RES_NOW; } inline int64_t nowStamp(short& i) { if (std::chrono::steady_clock::now() - RES_last < std::chrono::seconds(1)) return mktime(RES_NOW) + i; time_t rawtime; std::time(&rawtime);
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(RES_NOW, &rawtime);
#else
 localtime_r(&rawtime, RES_NOW);
#endif
 RES_last = std::chrono::steady_clock::now(); return rawtime + i; } inline int64_t nowStamp(short&& i) { if (std::chrono::steady_clock::now() - RES_last < std::chrono::seconds(1)) return mktime(RES_NOW) + i; time_t rawtime; std::time(&rawtime);
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(RES_NOW, &rawtime);
#else
 localtime_r(&rawtime, RES_NOW);
#endif
 RES_last = std::chrono::steady_clock::now(); return rawtime + i; } inline int64_t nowStamp() { if (std::chrono::steady_clock::now() - RES_last < std::chrono::seconds(1)) return mktime(RES_NOW); time_t rawtime; std::time(&rawtime);
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(RES_NOW, &rawtime);
#else
 localtime_r(&rawtime, RES_NOW);
#endif
 RES_last = std::chrono::steady_clock::now(); return rawtime; } namespace detail { static std::string directory_ = STATIC_DIRECTORY;  class dumb_timer_queue { public: static unsigned short tick; dumb_timer_queue(boost::asio::io_service& io_service) : io_service_(io_service), deadline_timer_(io_service_) {
#ifdef _WIN32
 deadline_timer_.expires_from_now(boost::posix_time::millseconds(1));
#else
 deadline_timer_.expires_from_now(boost::posix_time::seconds(1));
#endif
 deadline_timer_.async_wait(std::bind(&dumb_timer_queue::tick_handler, this, std::placeholders::_1)); } ~dumb_timer_queue() { deadline_timer_.cancel(); } inline void cancel(size_t id) { dq_.erase(id); } inline size_t add(const std::function<void()>& task) { dq_.insert({ ++step_, {std::chrono::steady_clock::now() + std::chrono::seconds(tick), task} }); return step_; } inline size_t add(const std::function<void()>& task, std::uint8_t& timeout) { dq_.insert({ ++step_, {std::chrono::steady_clock::now() + std::chrono::seconds(timeout), task} }); return step_; } inline size_t add(const std::function<void()>& task, std::uint8_t&& timeout) { dq_.insert({ ++step_, {std::chrono::steady_clock::now() + std::chrono::seconds(timeout), task} }); return step_; } private: void tick_handler(const boost::system::error_code& ec) { if (ec) return;
#ifdef _WIN32
 for (const auto& task : dq_) { if (task.second.first < std::chrono::steady_clock::now()) { (task.second.second)(); dq_.erase(task.first); } }
#else
 std::vector<uint16_t> vts; for (const auto& task : dq_) { if (task.second.first < std::chrono::steady_clock::now()) { (task.second.second)(); vts.push_back(task.first); } } for (const auto& task : vts) dq_.erase(task);
#endif
 deadline_timer_.expires_from_now(boost::posix_time::seconds(1)); deadline_timer_.async_wait(std::bind(&dumb_timer_queue::tick_handler, this, std::placeholders::_1)); } private: boost::asio::io_service& io_service_; boost::asio::deadline_timer deadline_timer_; std::map<uint16_t, std::pair<std::chrono::steady_clock::time_point, std::function<void()>>> dq_; uint16_t step_{ 0 }; }; } static std::string Time2Str(const time_t* t) { tm* _v; _v = std::localtime(t); std::ostringstream os; os << std::setfill('0'); std::string s;
#ifdef _WIN32
 os << std::setw(4) << _v->tm_year + 1900;
#else
 int y = _v->tm_year / 100; os << std::setw(2) << 19 + y << std::setw(2) << _v->tm_year - y * 100;
#endif
 os << '-' << std::setw(2) << (_v->tm_mon + 1) << '-' << std::setw(2) << _v->tm_mday << ' ' << std::setw(2) << _v->tm_hour << ':' << std::setw(2) << _v->tm_min << ':' << std::setw(2) << _v->tm_sec; s = os.str(); return s;}}