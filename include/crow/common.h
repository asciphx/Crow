#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "crow/str.h"
namespace crow {
  enum class HTTPMethod {
	DEL = 0, GET, HEAD, POST, PUT, CONNECT, OPTIONS,
	TRACE, PATCH, PURGE, InternalMethodCount,
	// should not add an item below this line: used for array count
  };
  inline std::string m2s(HTTPMethod m) {
	switch (m) {
	case HTTPMethod::DEL:return "DELETE";
	case HTTPMethod::GET:return "GET";
	case HTTPMethod::HEAD:return "HEAD";
	case HTTPMethod::POST:return "POST";
	case HTTPMethod::PUT:return "PUT";
	case HTTPMethod::CONNECT:return "CONNECT";
	case HTTPMethod::OPTIONS:return "OPTIONS";
	case HTTPMethod::TRACE:return "TRACE";
	case HTTPMethod::PATCH:return "PATCH";
	case HTTPMethod::PURGE:return "PURGE";
	default:return "invalid";
	}
	return "invalid";
  }
  inline HTTPMethod c2m(const char* m) {
	switch (hack8Str(m)) {
	case hack8Str("DELETE"):return crow::HTTPMethod::DEL;
	case 4670804:return crow::HTTPMethod::GET;
	case 1212498244:return crow::HTTPMethod::HEAD;
	case 1347375956:return crow::HTTPMethod::POST;
	case 5264724:return crow::HTTPMethod::PUT;
	case hack8Str("OPTIONS"):return crow::HTTPMethod::OPTIONS;
	case hack8Str("CONNECT"):return crow::HTTPMethod::CONNECT;
	case hack8Str("TRACE"):return crow::HTTPMethod::TRACE;
	case hack8Str("PATCH"):return crow::HTTPMethod::PATCH;
	case hack8Str("PURGE"):return crow::HTTPMethod::PURGE;
	}
	return HTTPMethod::InternalMethodCount;
  }
  enum class ParamType {
	INT, UINT, DOUBLE,
	STRING, PATH, MAX
  };
  struct routing_params {
	std::vector<int64_t> int_params;
	std::vector<uint64_t> uint_params;
	std::vector<double> double_params;
	std::vector<std::string> string_params;
	void debug_print() const {
	  std::cerr << "routing_params" << std::endl;
	  for (auto i : int_params) std::cerr << i << ", ";
	  std::cerr << std::endl;
	  for (auto i : uint_params) std::cerr << i << ", ";
	  std::cerr << std::endl;
	  for (auto i : double_params) std::cerr << i << ", ";
	  std::cerr << std::endl;
	  for (auto& i : string_params) std::cerr << i << ", ";
	  std::cerr << std::endl;
	}
	template <typename T>T get(unsigned) const;
  };
  template<>
  inline int64_t routing_params::get<int64_t>(unsigned index) const { return int_params[index]; }
  template<>
  inline uint64_t routing_params::get<uint64_t>(unsigned index) const { return uint_params[index]; }
  template<>
  inline double routing_params::get<double>(unsigned index) const { return double_params[index]; }
  template<>
  inline std::string routing_params::get<std::string>(unsigned index) const { return string_params[index]; }
}
#ifndef CROW_MSVC_WORKAROUND
constexpr crow::HTTPMethod operator""_mt(const char* str, size_t /*len*/) {
  switch (hack8Str(str)) {
  case hack8Str("DELETE"):return crow::HTTPMethod::DEL;
  case 'GET':return crow::HTTPMethod::GET;
  case 'HEAD':return crow::HTTPMethod::HEAD;
  case 'POST':return crow::HTTPMethod::POST;
  case 'PUT':return crow::HTTPMethod::PUT;
  case hack8Str("OPTIONS"):return crow::HTTPMethod::OPTIONS;
  case hack8Str("CONNECT"):return crow::HTTPMethod::CONNECT;
  case hack8Str("TRACE"):return crow::HTTPMethod::TRACE;
  case hack8Str("PATCH"):return crow::HTTPMethod::PATCH;
  case hack8Str("PURGE"):return crow::HTTPMethod::PURGE;
  }
  throw std::runtime_error("invalid http method");
}
#endif
