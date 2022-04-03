#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <set>
#include "crow/http_request.h"
namespace crow {
  using namespace std; namespace fs = std::filesystem;
  static std::set<const char*> RES_menu = {}; static const string crlf = "\r\n";//"\r\n\r\n"
  struct param { uint32_t size = 0; string key; string value; string filename;/* string type;*/ };
  ///The parsed multipart Req/Res (Length[ kb ]),(Bool is_MD5)
  template<unsigned short L, bool B = false>
  struct Parser {
	const ci_map* headers; string boundary, menu; vector<param> params; //string content_type = "multipart/form-data";
	const string& get_header_value(const string& key) const { return crow::get_header_value(*headers, key); }
	~Parser() { headers = nullptr; }
	Parser(const Req& req, const char* m) : headers(&(req.headers)), menu(CROW_UPLOAD_DIRECTORY),
	  boundary(g_b(get_header_value("Content-Type"))) {
	  menu += m; if (RES_menu.find(m) == RES_menu.end()) {
		RES_menu.insert(m); if (!fs::is_directory(menu)) { fs::create_directory(menu); }
	  }
	  params = p_b(req.body);
	}
	Parser(const Req& req) : headers(&(req.headers)), menu(CROW_UPLOAD_DIRECTORY),
	  boundary(g_b(get_header_value("Content-Type"))), params(p_b(req.body)) {}
	// Parser(const ci_map& headers, const string& boundary, const vector<param>& sections)
	   //: headers(&headers), boundary(boundary), params(sections) {}
  private: //get_boundary
	string g_b(const string& h) const {
	  //std::cout << "<" << h << ">" << h.size() << std::endl;
	  if (h.size() == 33) { return h.substr(0xc); }//application/x-www-form-urlencoded
	  size_t f = h.find("=----"); if (f) return h.substr(f + 0xe); return h;//raw
	}
	//parse_body
	vector<param> p_b(string value) {
	  if (boundary[0] == 'x') {
		throw std::runtime_error(value);
		throw std::runtime_error("Wrong application/x-www-form-urlencoded!");
	  } else if (boundary[0] == 'a') {
		try {
		  throw std::runtime_error(json::parse(value).dump());
		} catch (const std::exception&) {
		  throw std::runtime_error("Wrong json string!");
		}
	  }
	  if (value.size() < 45) throw std::runtime_error("Wrong value size!");
	  if (value.size() > L * 1024) throw std::runtime_error(std::string("Body size can't be biger than : ") + std::to_string(L) + "kb");
	  vector<param> sections; size_t f = value.find(boundary);
	  value.erase(0, f + boundary.length() + 2); string s; _:;
	  if (value.size() != 2) {
		f = value.find(boundary);
		s = value.substr(0, f - 0xf);
		sections.emplace_back(p_s(s));
		value.erase(0, f + boundary.length() + 2); goto _;
	  }
	  if (sections.size() == 0) throw std::runtime_error("Not Found!");
	  return sections;
	}
	//parse_section
	param p_s(string& s) {
	  struct param p;
	  size_t f = s.find("\r\n\r\n");
	  string lines = s.substr(0, f + 2);
	  s.erase(0, f + 4);
	  f = lines.find(';');
	  if (f != string::npos) lines.erase(0, f + 2);
	  f = lines.find(crlf);
	  string line = lines.substr(0, f);
	  lines.erase(0, f + 2);
	  char b = 0;
	  while (!line.empty()) {
		f = line.find(';');
		string value = line.substr(0, f);
		if (f != string::npos) line.erase(0, f + 2); else line.clear();
		f = value.find('=');
		value = value.substr(f + 2); value.pop_back();//(f + 1)
		if (b == '\0') {
		  p.key = value; ++b;
		} else if (b == '\1') {
		  p.filename = menu + value;//trim(value)
		  ++b;
		}
	  }
	  p.value = s.substr(0, s.length() - 2);
	  if (b == '\2') {
		f = lines.find(crlf);
		line = lines.substr(0, f);
		lines.erase(0, f + 2);
		f = line.rfind(';');
		string h = line.substr(0, f);
		if (f != string::npos) line.erase(0, f + 2); else line.clear();
		f = h.find(':');
		//p.type = h.substr(f + 2);
		p.size = p.value.length();
		std::ofstream of(CROW_STATIC_DIRECTORY + p.filename, ios::out | ios::app | ios::binary);
		of << p.value; of.close();
	  }
	  return p;
	}
	//inline string trim(string& string, const char& excess = '"') const {
	//  if (string[0] == excess && string[string.length() - 1] == excess)
	//	return string.substr(1, string.length() - 2);//string.length() > 1 && 
	//  return string;
	//}
  };
}
