#include "cc.h"
#include <sstream>
using namespace cc;using namespace std;
int main() {
  App<> app;app.directory(".").home("main.html").file_type({"html","css","js","json","wasm","ico"});
  app("/plaintext")([] {
	return "Hello, World!";
  });
  app("/json")([] {
	json x;
	x["message"]="Hello, World!";
	return x;
  });
  app.loglevel(LogLevel::CRITICAL).set_port(8080).multithreaded().run();
}