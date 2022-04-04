#pragma once
#include <string>
#include <unordered_map>
#include "cc/json.hh"
#include "cc/http_request.h"
#include "cc/any_types.h"
#include "cc/ci_map.h"
#include "cc/detail.h"
//response
static char RES_CT[13] = "Content-Type", RES_CL[15] = "Content-Length", RES_Loc[9] = "Location", Res_Ca[14] = "Cache-Control",
RES_AJ[17] = "application/json", RES_Txt[24] = "text/html;charset=UTF-8", RES_Xc[23] = "X-Content-Type-Options", RES_No[8] = "nosniff";
namespace cc {
  template <typename Adaptor, typename Handler, typename ... Middlewares>
  class Connection;
  struct Res {
  private:
	ci_map headers;
  public:
	template <typename Adaptor, typename Handler, typename ... Middlewares>
	friend class cc::Connection;
	uint16_t code{ 200 };// Check whether the response has a static file defined.
	std::string body; uint8_t is_file{ 0 };
	json json_value;
	// `headers' stores HTTP default headers.
#ifdef ENABLE_COMPRESSION
	bool compressed = true; //< If compression is enabled and this is false, the individual response will not be compressed.
#endif
	bool is_head_response = false;      ///< Whether this is a Res to a HEAD Req.
	inline void set_header(const char* key, std::string value) { headers.erase(key); headers.emplace(key, std::move(value)); }
	inline void set_header(std::string key, std::string value) { headers.erase(key); headers.emplace(std::move(key), std::move(value)); }
	inline void add_header(const char* key, std::string value) { headers.emplace(key, std::move(value)); }
	inline void add_header(const char* key, std::string_view value) { headers.emplace(key, value); }
	const std::string& get_header_value(const std::string& key) { return cc::get_header_value(headers, key); }
	Res() {}
	explicit Res(int code) : code(code) {}
	Res(std::string body) : body(std::move(body)) {}
	Res(int code, std::string body) : code(code), body(std::move(body)) {}
	Res(const json&& json_value) : body(std::move(json_value).dump()) {
	  headers.emplace(RES_CT, RES_AJ);
	}
	Res(int code, json& json_value) : code(code), body(json_value.dump()) {
	  headers.emplace(RES_CT, RES_AJ);
	}
	Res(const char*&& char_value) : body(std::move(char_value)) {}
	Res(int code, const char*&& char_value) : code(code), body(std::move(char_value)) {}
	Res(Res&& r) { *this = std::move(r); }
	Res& operator = (const Res& r) = delete;
	Res& operator = (Res&& r) noexcept {
	  body = std::move(r.body);
	  json_value = std::move(r.json_value);
	  code = r.code;
	  headers = std::move(r.headers);
	  path_ = std::move(r.path_);
	  completed_ = r.completed_;
	  return *this;
	}
	inline bool is_completed() const noexcept { return completed_; }
	inline void clear() { completed_ = false; body.clear(); headers.clear(); }
	/// Return a "Temporary Redirect" Res.
	/// Location can either be a route or a full URL.
	inline void redirect(const std::string& location) {
	  code = 301; headers.erase(RES_Loc);
	  headers.emplace(RES_Loc, std::move(location));
	}
	/// Return a "See Other" Res.
	inline void redirect_perm(const std::string& location) {
	  code = 303; headers.erase(RES_Loc);
	  headers.emplace(RES_Loc, std::move(location));
	}
	void write(const std::string& body_part) { body += body_part; }
	inline void end() {
	  if (!completed_) {
		completed_ = true;
		if (is_head_response) {
		  set_header(RES_CL, std::to_string(body.size()));
		  body = "";
		}
		if (complete_request_handler_) {
		  complete_request_handler_();
		}
	  }
	}
	inline void end(const std::string& body_part) { body += body_part; end(); }
	inline bool is_alive() { return is_alive_helper_ && is_alive_helper_(); }
	///Return a static file as the response body
	void set_static_file_info(std::string path) {
	  struct stat statbuf_; path_ = detail::directory_ + DecodeURL(path);
	  statResult_ = stat(path_.c_str(), &statbuf_);
#ifdef ENABLE_COMPRESSION
	  compressed = false;
#endif
	  if (statResult_ == 0) {
		std::size_t last_dot = path.find_last_of(".");
		std::string extension(path.substr(last_dot + 1));
		this->add_header(RES_CL, std::to_string(statbuf_.st_size));
		if (content_types.find(extension) != content_types.end()) {
		  this->add_header(RES_CT, content_types[extension]), is_file = 1;
		} else {
		  code = 404; this->headers.clear(); this->end();
		  //this->add_header(RES_CT,"text/plain");
		}
	  } else {
		code = 404;
	  }
	}
  private:
	std::string path_;
	int statResult_;
	bool completed_{};
	std::function<void()> complete_request_handler_;
	std::function<bool()> is_alive_helper_;

  };
}
