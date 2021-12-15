#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include "crow/mustache.h"
#include "crow/json.hh"
using namespace std;
using namespace crow;
using namespace crow::mustache;

string read_all(const string& filename) {
  ifstream is(filename);
  return {istreambuf_iterator<char>(is), istreambuf_iterator<char>()};
}

int main() {
  auto data=json::parse(read_all("data"));
  auto templ=template_t(read_all("template"));
  auto partials=json::parse(read_all("partials"));
  json ctx(data);
  cout<<templ.render(ctx);
  return 0;
}
