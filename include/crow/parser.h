#pragma once

#include <string>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <algorithm>

#include "crow/http_parser_merged.h"
#include "crow/http_request.h"
namespace crow {
  template <typename Handler>
  struct HTTPParser : public http_parser {
	static int on_url(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  self->clear();
	  self->raw_url.insert(self->raw_url.end(),at,at+length);
	  return 0;
	}
	static int on_header_field(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  switch (self->header_state) {
		case 0:
		if (!self->header_value.empty()) {
		  self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
		}
		self->header_field.assign(at,at+length);
		self->header_state=1;
		break;
		case 1:
		self->header_field.insert(self->header_field.end(),at,at+length);
		break;
	  }
	  return 0;
	}
	static int on_header_value(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  switch (self->header_state) {
		case 0:
		self->header_value.insert(self->header_value.end(),at,at+length);
		break;
		case 1:
		self->header_state=0;
		self->header_value.assign(at,at+length);
		break;
	  }
	  return 0;
	}
	static int on_headers_complete(http_parser* self_) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  if (!self->header_field.empty()) {
		self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
	  }
	  self->handler_->handle_header();
	  return 0;
	}
	static int on_body(http_parser* self_,const char* at,size_t length) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  self->body.insert(self->body.end(),at,at+length);
	  return 0;
	}
	static int on_message_complete(http_parser* self_) {
	  HTTPParser* self=static_cast<HTTPParser*>(self_);
	  // url params
	  self->url=self->raw_url.substr(0,self->raw_url.find("?"));
	  self->url_params=query_string(self->raw_url);
	  self->handler_->handle();
	  return 0;
	}
	HTTPParser(Handler* handler):handler_(handler) {
	  http_parser_init(this,HTTP_REQUEST);
	}
	// return false on error
	bool feed(const char* buffer,int length) {
	  const static http_parser_settings settings_{
		  nullptr,
		  on_url,
		  nullptr,
		  on_header_field,
		  on_header_value,
		  on_headers_complete,
		  on_body,
		  on_message_complete,
	  };
	  return http_parser_execute(this,&settings_,buffer,length)==length;
	}
	void clear() {
	  this->url.clear();
	  this->raw_url.clear();
	  this->header_field.clear();
	  this->header_value.clear();
	  this->headers.clear();
	  this->url_params.clear();
	  this->body.clear();
	  this->header_state=0;
	}
	//printf("url: %s\n",url.data());
	//printf("raw_url: %s\n",raw_url.data());
	//std::cout<<url_params<<std::endl;
	//
	Req to_request() const {
	  return Req{static_cast<HTTPMethod>(this->method), std::move(raw_url), std::move(url), std::move(url_params), std::move(headers), std::move(body)};
	}
	//
	bool is_upgrade() const { return this->upgrade; }
	//
	bool check_version(int major,int minor) const { return this->http_major==major&&this->http_minor==minor; }

	std::string raw_url;
	std::string url;
	int header_state;
	std::string header_field;
	std::string header_value;
	ci_map headers;
	query_string url_params;
	std::string body;

	Handler* handler_;
  };
}
