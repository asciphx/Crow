#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "crow/http_request.h"
namespace crow {
  using namespace std;
  struct returnable {
	string content_type;
	virtual string dump() const=0;
	returnable(string ctype): content_type{ctype} {}
	virtual ~returnable() {};
  };
  ///Encapsulates anything related to processing and organizing `multipart/xyz` messages
  namespace multipart {
	const string dd="--";
	const string crlf="\r\n";
	///The first part in a section, contains metadata about the part
	struct header {
	  pair<string,string> value; ///< The first part of the header, usually `Content-Type` or `Content-Disposition`
	  unordered_map<string,string> params; ///< The parameters of the header, come after the `value`
	};
	///One part of the multipart message
	///It is usually separated from other sections by a `boundary`
	///
	struct part {
	  vector<header> headers; ///< (optional) The first part before the data, Contains information regarding the type of data and encoding
	  string body; ///< The actual data in the part
	};

	///The parsed multipart Req/Res
	struct message : public returnable {
	  ci_map headers;
	  string boundary; ///< The text boundary that separates different `parts`
	  vector<part> parts; ///< The individual parts of the message

	  const string& get_header_value(const string& key) const {
		return crow::get_header_value(headers,key);
	  }

	  ///Represent all parts as a string (**does not include message headers**)
	  string dump() const override {
		stringstream str;
		string delimiter=dd+boundary;

		for (unsigned i=0; i<parts.size(); i++) {
		  str<<delimiter<<crlf;
		  str<<dump(i);
		}
		str<<delimiter<<dd<<crlf;
		return str.str();
	  }

	  ///Represent an individual part as a string
	  string dump(int part_) const {
		stringstream str;
		part item=parts[part_];
		for (header item_h:item.headers) {
		  str<<item_h.value.first<<": "<<item_h.value.second;
		  for (auto& it:item_h.params) {
			str<<"; "<<it.first<<'='<<pad(it.second);
		  }
		  str<<crlf;
		}
		str<<crlf;
		str<<item.body<<crlf;
		return str.str();
	  }

	  ///Default constructor using default values
	  message(const ci_map& headers,const string& boundary,const vector<part>& sections)
		: returnable("multipart/form-data"),headers(headers),boundary(boundary),parts(sections) {}

	  ///Create a multipart message from a Req data
	  message(const Req& req)
		: returnable("multipart/form-data"),
		headers(req.headers),
		boundary(get_boundary(get_header_value("Content-Type"))),
		parts(parse_body(req.body)) {}

	  private:

	  string get_boundary(const string& header) const {
		size_t found=header.find("boundary=");
		if (found)
		  return header.substr(found+9);
		return string();
	  }
	  vector<part> parse_body(string body) {
		vector<part> sections;
		string delimiter=dd+boundary;
		while (body!=(crlf)) {
		  size_t found=body.find(delimiter);
		  string section=body.substr(0,found);
		  //+2 is the CRLF
		  //We don't check it and delete it so that the same delimiter can be used for
		  //the last delimiter (--delimiter--CRLF).
		  body.erase(0,found+delimiter.length()+2);
		  if (!section.empty()) {
			sections.emplace_back(parse_section(section));
		  }
		}
		return sections;
	  }
	  part parse_section(string& section) {
		struct part to_return;
		size_t found=section.find(crlf+crlf);
		string head_line=section.substr(0,found+2);
		section.erase(0,found+4);
		parse_section_head(head_line,to_return);
		to_return.body=section.substr(0,section.length()-2);
		return to_return;
	  }

	  void parse_section_head(string& lines,part& part) {
		while (!lines.empty()) {
		  header to_add;
		  size_t found=lines.find(crlf);
		  string line=lines.substr(0,found);
		  lines.erase(0,found+2);
		  //add the header if available
		  if (!line.empty()) {
			size_t found=line.find("; ");
			string header=line.substr(0,found);
			if (found!=string::npos)
			  line.erase(0,found+2);
			else
			  line=string();
			size_t header_split=header.find(": ");
			to_add.value=pair<string,string>(header.substr(0,header_split),header.substr(header_split+2));
		  }
		  //add the parameters
		  while (!line.empty()) {
			size_t found=line.find("; ");
			string param=line.substr(0,found);
			if (found!=string::npos)
			  line.erase(0,found+2);
			else
			  line=string();
			size_t param_split=param.find('=');
			string value=param.substr(param_split+1);
			to_add.params.emplace(param.substr(0,param_split),trim(value));
		  }
		  part.headers.emplace_back(to_add);
		}
	  }

	  inline string trim(string& string,const char& excess='"') const {
		if (string.length()>1&&string[0]==excess&&string[string.length()-1]==excess)
		  return string.substr(1,string.length()-2);
		return string;
	  }
	  inline string pad(string& string,const char& padding='"') const {
		return (padding+string+padding);
	  }

	};
  }
}
