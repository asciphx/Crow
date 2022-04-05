
#pragma once
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <future>
#include <cstdint>
#include <type_traits>
#include <thread>
#include <condition_variable>

#include "cc/settings.h"
#include "cc/logging.h"
#include "cc/utility.h"
#include "cc/routing.h"
#include "cc/middleware_context.h"
#include "cc/http_request.h"
#include "cc/http_server.h"
#include "cc/detail.h"
#include "cc/file.h"
#include "cc/mustache.h"
#ifdef ENABLE_COMPRESSION
#include "cc/compression.h"
#endif

#ifdef _WIN32
#include <locale.h>
#define ROUTE(app, url) app.route(url)
#else
#define ROUTE(app, url) app.route_url<cc::spell::get_parameter_tag(url)>(url)
#endif
#define CATCHALL_ROUTE(app) app.default_route()

namespace cc { static std::string RES_home = HOME_PAGE; unsigned short detail::dumb_timer_queue::tick = 4;
#ifdef ENABLE_SSL
 using ssl_context_t = boost::asio::ssl::context;
#endif
   template <typename ... Middlewares> class Crow { public: using self_t = Crow; using server_t = Server<Crow, SocketAdaptor, Middlewares...>;
#ifdef ENABLE_SSL
  using ssl_server_t = Server<Crow, SSLAdaptor, Middlewares...>;
#endif
 Crow() {
#ifdef _WIN32
 ::system("chcp 65001 >nul"); setlocale(LC_CTYPE, ".UTF8");
#else
 std::locale::global(std::locale("en_US.UTF8"));
#endif
 std::cout << "C++ web[服务] run on http://localhost"; }   template <typename Adaptor> void handle_upgrade(const Req& req, Res& res, Adaptor&& adaptor) { router_.handle_upgrade(req, res, adaptor); }  void handle(const Req& req, Res& res) { router_.handle(req, res); }  DynamicRule& route(std::string&& rule) { return router_.new_rule_dynamic(std::move(rule)); }  template <uint64_t Tag> auto route_url(std::string&& rule) -> typename std::result_of<decltype(&Router::new_rule_tagged<Tag>)(Router, std::string&&)>::type { return router_.new_rule_tagged<Tag>(std::move(rule)); }  CatchallRule& default_route() { return router_.catchall_rule(); } self_t& signal_clear() { signals_.clear(); return *this; } self_t& signal_add(int signal_number) { signals_.push_back(signal_number); return *this; }  self_t& port(std::uint16_t port) { port_ = port; std::cout << ":" << port_ << std::endl; return *this; }  self_t& timeout(std::uint8_t timeout) { if (timeout > 10)timeout = 10; if (timeout < 1)timeout = 1; detail::dumb_timer_queue::tick = timeout; return *this; }  self_t& bindaddr(std::string bindaddr) { bindaddr_ = bindaddr; return *this; }  self_t& directory(std::string path) { if (path.back() != '\\' && path.back() != '/') path += '/'; detail::directory_ = path; return *this; } self_t& home(std::string path) { RES_home = path; return *this; }  self_t& file_type(const std::vector<std::string_view>& line) { for (auto iter = line.cbegin(); iter != line.cend(); ++iter) { std::string_view types; types = content_any_types[*iter]; if (types != "") { content_types.emplace(*iter, types); } } is_not_set_types = false; return *this; }  self_t& multithreaded() { return concurrency(std::thread::hardware_concurrency()); }  self_t& concurrency(std::uint16_t concurrency) { concurrency_ = concurrency; return *this; }       self_t& loglevel(cc::LogLevel level) { cc::logger::setLogLevel(level); return *this; }
#ifdef ENABLE_COMPRESSION
 self_t& use_compression(compression::algorithm algorithm) { comp_algorithm_ = algorithm; return *this; } compression::algorithm compression_algorithm() { return comp_algorithm_; }
#endif
   void validate() { router_.validate(); }  void notify_server_start() { std::unique_lock<std::mutex> lock(start_mutex_); server_started_ = true; cv_started_.notify_all(); }  void run() { if (is_not_set_types) { this->file_type({ "html","ico","css","js","json","svg","png","jpg","gif","txt" }); is_not_set_types = false; }
#ifndef DISABLE_HOME
 route_url<cc::spell::get_parameter_tag("/")>("/")([] { return (std::string)mustache::load(RES_home); });
#endif
 validate();
#ifdef ENABLE_SSL
 if (use_ssl_) { ssl_server_ = std::move(std::unique_ptr<ssl_server_t>(new ssl_server_t(this, bindaddr_, port_, &middlewares_, concurrency_, &ssl_context_))); notify_server_start(); ssl_server_->run(); } else
#endif
 { server_ = std::move(std::unique_ptr<server_t>(new server_t(this, bindaddr_, port_, &middlewares_, concurrency_, nullptr))); server_->signal_clear(); for (auto snum : signals_) { server_->signal_add(snum); } notify_server_start(); server_->run(); } }  void stop() {
#ifdef ENABLE_SSL
 if (use_ssl_) { if (ssl_server_) { ssl_server_->stop(); } } else
#endif
 { if (server_) { server_->stop(); } } } void debug_print() { LOG_DEBUG << "Routing:"; router_.debug_print(); }
#ifdef ENABLE_SSL
  self_t& ssl_file(const std::string& crt_filename, const std::string& key_filename) { use_ssl_ = true; ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer); ssl_context_.set_verify_mode(boost::asio::ssl::verify_client_once); ssl_context_.use_certificate_file(crt_filename, ssl_context_t::pem); ssl_context_.use_private_key_file(key_filename, ssl_context_t::pem); ssl_context_.set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 ); return *this; }  self_t& ssl_file(const std::string& pem_filename) { use_ssl_ = true; ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer); ssl_context_.set_verify_mode(boost::asio::ssl::verify_client_once); ssl_context_.load_verify_file(pem_filename); ssl_context_.set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 ); return *this; } self_t& ssl(boost::asio::ssl::context&& ctx) { use_ssl_ = true; ssl_context_ = std::move(ctx); return *this; } bool use_ssl_{ false }; ssl_context_t ssl_context_{ boost::asio::ssl::context::sslv23 };
#else
 template <typename T, typename ... Remain> self_t& ssl_file(T&&, Remain&&...) {  static_assert(  std::is_base_of<T, void>::value, "Define ENABLE_SSL to enable ssl support."); return *this; } template <typename T> self_t& ssl(T&&) {  static_assert(  std::is_base_of<T, void>::value, "Define ENABLE_SSL to enable ssl support."); return *this; }
#endif
  using context_t = detail::Ctx<Middlewares...>; template <typename T> typename T::Ctx& get_context(const Req& req) { static_assert(spell::contains<T, Middlewares...>::value, "App doesn't have the specified middleware type."); auto& ctx = *reinterpret_cast<context_t*>(req.middleware_context); return ctx.template get<T>(); } template <typename T> T& get_middleware() { return utility::get_element_by_type<T, Middlewares...>(middlewares_); }  void wait_for_server_start() { std::unique_lock<std::mutex> lock(start_mutex_); if (server_started_) return; cv_started_.wait(lock); } private: uint16_t port_ = DEFAULT_PORT; uint16_t concurrency_ = 1; std::string bindaddr_ = "0.0.0.0"; Router router_; bool is_not_set_types = true;
#ifdef ENABLE_COMPRESSION
 compression::algorithm comp_algorithm_;
#endif
 std::tuple<Middlewares...> middlewares_;
#ifdef ENABLE_SSL
 std::unique_ptr<ssl_server_t> ssl_server_;
#endif
 std::unique_ptr<server_t> server_; std::vector<int> signals_{ SIGINT, SIGTERM }; bool server_started_{ false }; std::condition_variable cv_started_; std::mutex start_mutex_; }; template <typename ... Middlewares> using App = Crow<Middlewares...>;}