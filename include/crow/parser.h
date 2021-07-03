#pragma once
#include <string>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <algorithm>
//#include "crow/http_parser_merged.h"
#include "crow/picohttpparser.h"
#include "crow/http_request.h"
#define MAX_URL_LENGTH 99
namespace crow {
  //RESmethod=(char**)malloc(sizeof(char*)*MAX_CORE);RESpath=(char**)malloc(sizeof(char*)*MAX_CORE);
  template <typename Handler>
  struct HTTPParser {
	HTTPParser(Handler* handler):handler_(handler) {}
	bool feed(const char* buffer,int length) {
	  //int nparsed=http_parser_execute(this,buffer,length);
	  RESnum=sizeof(RESheader)/sizeof(RESheader[0]);
	  RESret=phr_parse_request(buffer,length,&RESmethod,&RESi,&RESpath,&RESl,&minor_version,RESheader,&RESnum,0);
	  if(RESret==length){
		RESj=0,RESk=0;char method[8],raw[MAX_URL_LENGTH];
		while (RESi--)method[RESj]=RESmethod[RESj],++RESj;method[RESj]=0;method_=m2i(method);
		while (RESl--)raw[RESk]=RESpath[RESk],++RESk;raw[RESk]=0;raw_url=raw;
		url=raw_url.substr(0,raw_url.find("?"));
		url_params=query_string(raw_url);
		for (RESi=0,RESl=0; RESnum--;) {
		  RESj=0,RESk=0;
		  RESi=RESheader[RESnum].name_len;
		  RESl=RESheader[RESnum].value_len;
		  const char *a=RESheader[RESnum].name;
		  const char *b=RESheader[RESnum].value;
		  char c[40],d[165];
		  while (RESj<RESi) c[RESj]=a[RESj],++RESj;c[RESj]=0;
		  while (RESk<RESl) d[RESk]=b[RESk],++RESk;d[RESk]=0;
		  //printf("%s: %s\n", c,d);
		  headers.emplace(c,d);
		}
		handler_->handle_header();
		handler_->handle();
		//printf("url: %s\n",url.data());std::cout<<url_params<<std::endl;
		//pp();
		url.clear();
		raw_url.clear();
		headers.clear();
		url_params.clear();
		body.clear();
		return true;
	  }
	  return false;
	}
	//void pp() {
	//  for (auto&ii:headers) std::cout<<ii.first<<" , "<<ii.second<<std::endl;
	//}
	Req to_request() const {
	  return Req{method_, std::move(raw_url), std::move(url),
		 std::move(url_params), std::move(headers), std::move(body)};
	}
	std::string raw_url;
	std::string url;
	ci_map headers;
	HTTPMethod method_;
	query_string url_params;
	std::string body;
	int minor_version;
	Handler* handler_;

	const char*RESmethod,*RESpath;
	size_t RESi,RESl,RESj,RESk,RESnum;
	struct phr_header RESheader[16];
	int RESret;
  };
}
