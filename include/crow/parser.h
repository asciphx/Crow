#pragma once

#include <string>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <algorithm>

#include "crow/llhttp.h"
#include "crow/http_request.h"
typedef llhttp_t http_parser;
typedef llhttp_settings_t http_parser_settings;
namespace crow {
  template <typename Handler>
  struct HTTPParser : public http_parser {
	//static int on_message_begin(http_parser* self_) {
	//  HTTPParser* self=static_cast<HTTPParser*>(self_);
	//  return HPE_OK;
	//}
	static int on_url(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  self->raw_url.insert(self->raw_url.end(),at,at+length);
	  return HPE_OK;
	}
	static int on_header_field(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  switch (self->header_state) {
		case HPE_OK:
		if (!self->header_value.empty()) {
		  self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
		}
		self->header_field.assign(at,at+length);
		self->header_state=1;
		break;
		case HPE_INTERNAL:
		self->header_field.insert(self->header_field.end(),at,at+length);
		break;
	  }
	  return HPE_OK;
	}
	static int on_header_value(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  switch (self->header_state) {
		case HPE_OK:
		self->header_value.insert(self->header_value.end(),at,at+length);
		break;
		case HPE_INTERNAL:
		self->header_state=0;
		self->header_value.assign(at,at+length);
		break;
	  }
	  return HPE_OK;
	}
	static int on_headers_complete(http_parser* self_) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  if (!self->header_field.empty()) {
		self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
	  }self->handler_->handle_header();
	  return HPE_OK;
	}
	static int on_body(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  self->body.insert(self->body.end(),at,at+length);
	  return HPE_OK;
	}
	static int on_message_complete(http_parser* self_) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  // url params
	  self->url=self->raw_url.substr(0,self->raw_url.find("?"));
	  self->url_params=query_string(self->raw_url);
	  self->handler_->handle();
	  //self->url.clear();self->raw_url.clear();self->header_field.clear();
	  //self->header_value.clear();self->headers.clear();
	  //self->url_params.clear();self->body.clear();self->header_state=0;
	  return HPE_OK;
	}
	HTTPParser(Handler* handler):handler_(handler){
	  const static http_parser_settings settings_{
		  //on_message_begin,
		  nullptr,
		  on_url,
		  nullptr,
		  on_header_field,
		  on_header_value,
		  on_headers_complete,
		  on_body,
		  on_message_complete,
	  };
	  llhttp_init(this,HTTP_REQUEST,&settings_);
	}
	void clear(){
	}
	//~HTTPParser(){ delete parser_; }
	bool feed(const char* buffer,int length) {
	  return llhttp_execute(this,buffer,length)==length;
	}
	//
	Req to_request() const {
	  //printf("url: %s\n",url.data());
	  //printf("raw_url: %s\n",raw_url.data());
	  //std::cout<<url_params<<std::endl;
	  return Req{static_cast<HTTPMethod>(method), std::move(raw_url), std::move(url), std::move(url_params), std::move(headers), std::move(body)};
	}
	//
	bool is_upgrade() const { return upgrade; }
	//
	bool check_version(int major,int minor) const { return http_major==major&&http_minor==minor; }

	ci_map headers;
	private:
	std::string raw_url;
	std::string url;
	int header_state=0;
	std::string header_field;
	std::string header_value;
	query_string url_params;
	std::string body;

	//http_parser* parser_;
	http_parser_settings setting_;
	Handler* handler_;
  };
}
