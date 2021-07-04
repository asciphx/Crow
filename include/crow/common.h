#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "crow/utility.h"
namespace crow {
  enum class HTTPMethod {
	DEL=0,GET,HEAD,POST,PUT,CONNECT,OPTIONS,
	TRACE,PATCH,PURGE,InternalMethodCount,
	// should not add an item below this line: used for array count
  };
  constexpr unsigned long long fromStrL(const char*s) {
	unsigned long long r=0;for (int i=0;s[i];r*=256,r+=s[i++]);return r;
  }
  inline HTTPMethod m2i(std::string m) {
	switch (fromStrL(m.data())) {
	  case fromStrL("DELETE"):return HTTPMethod::DEL;
	  case 'GET':return HTTPMethod::GET;
	  case 'HEAD':return HTTPMethod::HEAD;
	  case 'POST':return HTTPMethod::POST;
	  case 'PUT':return HTTPMethod::PUT;
	}
	return HTTPMethod::GET;
  }
  inline std::string method_name(HTTPMethod m) {
	switch (m) {
	  case HTTPMethod::DEL:return "DELETE";
	  case HTTPMethod::GET:return "GET";
	  case HTTPMethod::HEAD:return "HEAD";
	  case HTTPMethod::POST:return "POST";
	  case HTTPMethod::PUT:return "PUT";
	  //case HTTPMethod::CONNECT:return "CONNECT";
	  case HTTPMethod::OPTIONS:return "OPTIONS";
	  //case HTTPMethod::TRACE:return "TRACE";
	  //case HTTPMethod::PATCH:return "PATCH";
	  //case HTTPMethod::PURGE:return "PURGE";
	}
	return "invalid";
  }
  enum class ParamType {
	INT,UINT,DOUBLE,
	STRING,PATH,MAX
  };
  struct routing_params {
	std::vector<int64_t> int_params;
	std::vector<uint64_t> uint_params;
	std::vector<double> double_params;
	std::vector<std::string> string_params;
	void debug_print() const {
	  std::cerr<<"routing_params"<<std::endl;
	  for (auto i:int_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto i:uint_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto i:double_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto& i:string_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
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
constexpr crow::HTTPMethod operator "" _method(const char* str,size_t /*len*/) {
  return
	crow::spell::is_equ_p(str,"GET",3)?crow::HTTPMethod::GET:
	crow::spell::is_equ_p(str,"DELETE",6)?crow::HTTPMethod::DEL:
	crow::spell::is_equ_p(str,"HEAD",4)?crow::HTTPMethod::HEAD:
	crow::spell::is_equ_p(str,"POST",4)?crow::HTTPMethod::POST:
	crow::spell::is_equ_p(str,"PUT",3)?crow::HTTPMethod::PUT:
	crow::spell::is_equ_p(str,"OPTIONS",7)?crow::HTTPMethod::OPTIONS:
	crow::spell::is_equ_p(str,"CONNECT",7)?crow::HTTPMethod::CONNECT:
	crow::spell::is_equ_p(str,"TRACE",5)?crow::HTTPMethod::TRACE:
	crow::spell::is_equ_p(str,"PATCH",5)?crow::HTTPMethod::PATCH:
	crow::spell::is_equ_p(str,"PURGE",5)?crow::HTTPMethod::PURGE:
	throw std::runtime_error("invalid http method");
}
#endif
