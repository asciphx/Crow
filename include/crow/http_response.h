#pragma once
#include <string>
#include <unordered_map>
#include "crow/json.hpp"
#include "crow/http_request.h"
#include "crow/any_types.h"
#include "crow/ci_map.h"
//response
static char RES_CT[13]="Content-Type",RES_CL[15]="Content-Length",RES_Loc[9]="Location", Res_Ca[14] = "Cache-Control",
  RES_AJ[17]="application/json", RES_Txt[24] = "text/html;charset=UTF-8", RES_Xc[23] = "X-Content-Type-Options", RES_No[8] = "nosniff";
//RES_f[6]="false",RES_Al[6]="Allow";
namespace crow {
  using json=nlohmann::json;
  template <typename Adaptor,typename Handler,typename ... Middlewares>
  class Connection;
  struct Res {
	private:
	ci_map headers;
	public:
	template <typename Adaptor,typename Handler,typename ... Middlewares>
	friend class crow::Connection;
	uint16_t code{200};// Check whether the response has a static file defined.
	std::string body; uint8_t  is_file{ 0 }, hType{ 0 };//1->txt,2->json,3->file
	json json_value;
	// `headers' stores HTTP default headers.
#ifdef CROW_ENABLE_COMPRESSION
	bool compressed=true; //< If compression is enabled and this is false, the individual response will not be compressed.
#endif
	bool is_head_response=false;      ///< Whether this is a Res to a HEAD Req.
	inline void set_header(std::string key,std::string value) {
	  headers.erase(key); headers.emplace(std::move(key),std::move(value));
	}
	inline void add_header(std::string key,std::string value) { headers.emplace(std::move(key),std::move(value)); }
	const std::string& get_header_value(const std::string& key) {
	  return crow::get_header_value(headers,key);
	}
	Res() {}
	explicit Res(int code): code(code) {}
	Res(std::string body): body(std::move(body)) { hType=1; }
	Res(int code,std::string body): code(code),body(std::move(body)) { hType=1; }
	Res(const json&& json_value): body(std::move(json_value).dump()) { hType=2; }
	Res(int code,json&json_value): code(code),body(json_value.dump()) { hType=2; }
	Res(const char* && char_value): body(std::move(char_value)) {hType=1;}
	Res(int code,const char* && char_value): code(code),body(std::move(char_value)) {hType=1;}
	Res(Res&& r) { *this=std::move(r); }
	Res& operator = (const Res& r)=delete;
	Res& operator = (Res&& r) noexcept {
	  body=std::move(r.body);
	  json_value=std::move(r.json_value);
	  code = r.code; hType = r.hType;
	  headers=std::move(r.headers);
	  path_=std::move(r.path_);
	  completed_=r.completed_;
	  return *this;
	}
	inline bool is_completed() const noexcept { return completed_; }
	inline void clear() { completed_=false;body.clear();headers.clear(); }
	/// Return a "Temporary Redirect" Res.
	/// Location can either be a route or a full URL.
	inline void redirect(const std::string& location) {
	  code=301;headers.erase(RES_Loc);
	  headers.emplace(RES_Loc,std::move(location));
	}
	/// Return a "See Other" Res.
	inline void redirect_perm(const std::string& location) {
	  code=303;headers.erase(RES_Loc);
	  headers.emplace(RES_Loc,std::move(location));
	}
	void write(const std::string& body_part) { body+=body_part; }
	inline void end() {
	  if (!completed_) {
		completed_=true;
		if (is_head_response) {
		  set_header(RES_CL,std::to_string(body.size()));
		  body="";
		}
		if (complete_request_handler_) {
		  complete_request_handler_();
		}
	  }
	}
	inline void end(const std::string& body_part) { body+=body_part; end(); }
	inline bool is_alive() { return is_alive_helper_&&is_alive_helper_();}
	///Return a static file as the response body
	void set_static_file_info(std::string path) {
	  struct stat statbuf_;path_=detail::directory_+path;
	  statResult_=stat(path_.c_str(),&statbuf_);
#ifdef CROW_ENABLE_COMPRESSION
	  compressed=false;
#endif
	  if (statResult_==0) {
		std::size_t last_dot=path.find_last_of(".");
		std::string extension=path.substr(last_dot+1);
		this->add_header(RES_CL,std::to_string(statbuf_.st_size));
		std::string types="";types=content_types[extension];
		if (types!="") {
		  this->add_header(RES_CT,types),is_file=1;
		  hType = 3; if (extension=="ico")hType = 0;
		} else {
		  code=404;this->headers.clear();this->end();
		  //this->add_header(RES_CT,"text/plain");
		}
	  } else {
		code=404;
	  }
	}
	/// Stream a static file.
	template<typename Adaptor>
	inline void do_stream_file(Adaptor& adaptor) {
	  if (statResult_==0) {
		std::ifstream is(path_.c_str(),std::ios::in|std::ios::binary);
		write_streamed(is,adaptor);
	  }
	}
	/// Stream the response body (send the body in chunks).
	template<typename Adaptor>
	inline void do_stream_body(Adaptor& adaptor) {
	  if (body.length()>0)write_streamed_string(body,adaptor);
	}
	private:
	std::string path_;
	int statResult_;
	bool completed_{};
	std::function<void()> complete_request_handler_;
	std::function<bool()> is_alive_helper_;
	template<typename Stream,typename Adaptor>
	void write_streamed(Stream& is,Adaptor& adaptor) {
	  char buf[16384];
	  while (is.read(buf,sizeof(buf)).gcount()>0) {
		std::vector<boost::asio::const_buffer> buffers;
		buffers.push_back(boost::asio::buffer(buf));
		write_buffer_list(buffers,adaptor);
	  }
	}

	//THIS METHOD DOES MODIFY THE BODY, AS IN IT EMPTIES IT
	template<typename Adaptor>
	inline void write_streamed_string(std::string& is,Adaptor& adaptor) {
	  std::string buf;
	  std::vector<boost::asio::const_buffer> buffers;
	  while (is.length()>16384) {
		//buf.reserve(16385);
		buf=is.substr(0,16384);
		is=is.substr(16384);
		push_and_write(buffers,buf,adaptor);
	  }
	  //Collect whatever is left (less than 16KB) and send it down the socket
	  //buf.reserve(is.length());
	  buf = is;
	  is.clear();
	  push_and_write(buffers,buf,adaptor);
	}

	template<typename Adaptor>
	inline void push_and_write(std::vector<boost::asio::const_buffer>& buffers,std::string& buf,Adaptor& adaptor) {
	  buffers.clear();
	  buffers.push_back(boost::asio::buffer(buf));
	  write_buffer_list(buffers,adaptor);
	}

	template<typename Adaptor>
	inline void write_buffer_list(std::vector<boost::asio::const_buffer>& buffers,Adaptor& adaptor) {
	  boost::asio::write(adaptor.socket(),buffers,[this](std::error_code ec,std::size_t) {
		if (!ec) {
		  return false;
		} else {
		  CROW_LOG_ERROR<<ec<<" - happened while sending buffers";
		  this->end();
		  return true;
		}
	  });
	}
  };
}
