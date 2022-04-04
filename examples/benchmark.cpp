#include "cc.h"
#include <sstream>
using namespace cc;using namespace std;
int main() {
  App<> app;app.directory(".").home("main.html").file_type({"html","css","js","json","wasm","ico"});
  app.route("/plaintext")([] {
	return "Hello, World!";
  });
  app.route("/json")([] {
	json x;
	x["message"]="Hello, World!";
	return x;
  });
  app.loglevel(LogLevel::WARNING).port(8080).multithreaded().run();
}