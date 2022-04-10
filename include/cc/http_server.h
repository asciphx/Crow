
#pragma once
#include <chrono>
#include <boost/asio.hpp>
#ifdef ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include <cstdint>
#include <atomic>
#include <future>
#include <vector>
#include <memory>
#include <filesystem>

#include "cc/http_connection.h"
#include "cc/logging.h"
#include "cc/detail.h"

namespace cc { static const char RES_GMT[26] = "%a, %d %b %Y %H:%M:%S GMT"; static uint16_t port_ = DEFAULT_PORT; static std::string bindaddr_ = "0.0.0.0"; using namespace boost; using tcp = asio::ip::tcp; template <typename Handler, typename Adaptor = SocketAdaptor, typename ... Middlewares> class Server { public: Server(Handler* handler, std::tuple<Middlewares...>* middlewares = nullptr, uint16_t concurrency = 1, typename Adaptor::Ctx* adaptor_ctx = nullptr) : acceptor_(io_service_, tcp::endpoint(boost::asio::ip::address::from_string(cc::bindaddr_), cc::port_)), signals_(io_service_, SIGINT, SIGTERM), handler_(handler), concurrency_(concurrency < 2 ? 2 : concurrency), roundrobin_index_(concurrency_ - 1), core_(concurrency_ - 1), middlewares_(middlewares), adaptor_ctx_(adaptor_ctx) {} void run() { if (!std::filesystem::is_directory(detail::directory_)) { std::filesystem::create_directory(detail::directory_); } if (!std::filesystem::is_directory(detail::directory_ + detail::upload_path_)) { std::filesystem::create_directory(detail::directory_ + detail::upload_path_); } for (int i = 0; i < concurrency_; ++i) io_service_pool_.emplace_back(new boost::asio::io_service()); get_cached_date_str_pool_.resize(concurrency_); timer_queue_pool_.resize(concurrency_); std::vector<std::future<void>> v; std::atomic<int> init_count(0); for (uint16_t i = 0; i < concurrency_; ++i) v.push_back( std::async(std::launch::async, [this, i, &init_count] { get_cached_date_str_pool_[i] = []()->std::string { if (std::chrono::steady_clock::now() - RES_last > std::chrono::seconds(2)) { time(&RES_TIME_T); RES_last = std::chrono::steady_clock::now();
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(RES_NOW, &RES_TIME_T);
#else
 localtime_r(&RES_TIME_T, RES_NOW);
#endif
 RES_DATE_STR.resize(0x30); RES_DATE_STR.resize(strftime(&RES_DATE_STR[0], 0x2f, RES_GMT, RES_NOW)); } return RES_DATE_STR; };  detail::dumb_timer_queue timer_queue(*io_service_pool_[i]); timer_queue_pool_[i] = &timer_queue; roundrobin_index_[i] = 0; ++init_count; while (1) { try { if (io_service_pool_[i]->run() == 0) {  break; } } catch (std::exception& e) { LOG_WARNING << "Worker Crash: " << e.what(); } } })); LOG_INFO << SERVER_NAME << " server is running at " << cc::bindaddr_ << ":" << acceptor_.local_endpoint().port() << " using " << concurrency_ << " threads"; LOG_INFO << "Call `app.loglevel(cc::LogLevel::Warning)` to hide Info level logs."; signals_.async_wait( [this](const boost::system::error_code& , int ) { stop(); }); while (concurrency_ != init_count) std::this_thread::yield(); do_accept(); std::thread([this] { io_service_.run(); LOG_INFO << "Exiting."; }).join(); } inline void stop() { io_service_.stop(); for (auto& io_service : io_service_pool_) io_service->stop(); } void signal_clear() { signals_.clear(); } void signal_add(int signal_number) { signals_.add(signal_number); } private: inline uint16_t pick_io_service() { if (roundrobin_index_[0] == 0)return 0; uint16_t i = 0, l = core_; while (++i < core_ && roundrobin_index_[l] < roundrobin_index_[i])l = i; return i; } inline void do_accept() { uint16_t idex = pick_io_service(); asio::io_service& is = *io_service_pool_[idex]; ++roundrobin_index_[idex]; auto p = new Connection<Adaptor, Handler, Middlewares...>( is, handler_, middlewares_, get_cached_date_str_pool_[idex], *timer_queue_pool_[idex], adaptor_ctx_, roundrobin_index_[idex]); acceptor_.async_accept(p->socket(), [this, p, &is](const boost::system::error_code& ec) { if (!ec) { is.post([p] { p->start(); }); } else delete p; do_accept(); }); } private: asio::io_service io_service_; std::vector<std::unique_ptr<asio::io_service>> io_service_pool_; std::vector<detail::dumb_timer_queue*> timer_queue_pool_; std::vector<std::function<std::string()>> get_cached_date_str_pool_; tcp::acceptor acceptor_; boost::asio::signal_set signals_; Handler* handler_; uint8_t concurrency_{ 1 }; uint8_t core_{ 1 }; std::vector<std::atomic<uint16_t>> roundrobin_index_; std::tuple<Middlewares...>* middlewares_;
#ifdef ENABLE_SSL
 bool use_ssl_{ false }; boost::asio::ssl::context ssl_context_{ boost::asio::ssl::context::sslv23 };
#endif
   typename Adaptor::Ctx* adaptor_ctx_; };}