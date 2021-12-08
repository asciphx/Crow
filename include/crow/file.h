#pragma once
#include <fstream>
#include <string>
#include <iterator>
#include "crow/detail.h"

namespace crow {
  namespace file {
	using namespace std;
	string read_file2s(string& filename) {
	  filename = detail::directory_+filename;
	  ifstream is(filename);filename.~basic_string();
	  if (!is) return {};
	  return {istreambuf_iterator<char>(is), istreambuf_iterator<char>()};
	}
  }
}