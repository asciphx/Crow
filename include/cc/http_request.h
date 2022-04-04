#pragma once
#include <boost/asio.hpp>
#include <string_view>
#include "cc/common.h"
#include "cc/ci_map.h"
#include "cc/query_string.h"
namespace cc {
  /// Find and return the value associated with the key. (returns an empty string if nothing is found)
  template <typename T>
  inline const std::string& get_header_value(const T& headers,const std::string& key) {
    if (headers.count(key)) return headers.find(key)->second;
    static std::string empty;
    return empty;
  }
  struct DetachHelper;
  struct Req {
    HTTP method;
    std::string raw_url; ///< The full URL containing the `?` and URL parameters.
    std::string url; ///< The endpoint without any parameters.
    query_string url_params; ///< The parameters associated with the request. (everything after the `?`)
    ci_map headers;
    std::string body;
    std::string remoteIpAddress; ///< The IP address from which the request was sent.
    void* middleware_context{};
    boost::asio::io_service* io_service{};

    /// Construct an empty Req. (sets the method to `GET`)
    Req(): method(HTTP::GET) {}

    /// Construct a Req with all values assigned.
    Req(HTTP method,std::string raw_url,std::string url,query_string url_params,ci_map headers,std::string body)
      : method(method),raw_url(std::move(raw_url)),url(std::move(url)),url_params(std::move(url_params)),headers(std::move(headers)),body(std::move(body)) {}

    void add_header(std::string key,std::string value) {
      headers.emplace(std::move(key), std::move(value));
    }

    const std::string& get_header_value(const std::string& key) const {
      return cc::get_header_value(headers,key);
    }

    /// Send the Req with a completion handler and return immediately.
    template<typename CompletionHandler>
    void post(CompletionHandler handler) {
      io_service->post(handler);
    }

    /// Send the Req with a completion handler.
    template<typename CompletionHandler>
    void dispatch(CompletionHandler handler) {
      io_service->dispatch(handler);
    }
  };
}
