
#pragma once

#include <boost/asio.hpp>

#include "cc/common.h"
#include "cc/ci_map.h"
#include "cc/query_string.h"

namespace cc { static const std::string RES_CT("Content-Type", 12), RES_CL("Content-Length", 14), RES_Loc("Location", 8), Res_Ca("Cache-Control", 13), RES_Cookie("Cookie", 6), RES_AJ("application/json", 16), RES_CALLBACK("CB", 2), RES_Txt("text/html;charset=UTF-8", 23), RES_Xc("X-Content-Type-Options", 22), RES_No("nosniff", 7), RES_Allow("Allow", 5);  template<typename T> inline const std::string& get_header_value(const T& headers, const std::string& key) { if (headers.count(key)) { return headers.find(key)->second; } static std::string empty; return empty; }  struct Req { HTTP method; std::string raw_url;   std::string url;   query_string url_params;  ci_map headers; std::string body; std::string remote_ip_address;  void* middleware_context{}; void* middleware_container{}; boost::asio::io_service* io_service{};  Req() : method(HTTP::GET) {}  Req(HTTP method, std::string raw_url, std::string url, query_string url_params, ci_map headers, std::string body) : method(method), raw_url(std::move(raw_url)), url(std::move(url)), url_params(std::move(url_params)), headers(std::move(headers)), body(std::move(body)) {} void add_header(std::string key, std::string value) { headers.emplace(std::move(key), std::move(value)); } const std::string& get_header_value(const std::string& key) const { return cc::get_header_value(headers, key); }  template<typename CompletionHandler> void post(CompletionHandler handler) { io_service->post(handler); }  template<typename CompletionHandler> void dispatch(CompletionHandler handler) { io_service->dispatch(handler); } };} 