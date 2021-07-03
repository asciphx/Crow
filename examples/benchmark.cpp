#include "crow.h"
#include "mustache.h"
#include <sstream>
using namespace crow;using namespace std;
int main() {
  App<> app;app.set_directory("./static").set_types({"html","css","js","json"});
  app.route("/plaintext")([] {
	return "Hello, World!";
  });
  app.route("/json")([] {
	json x;
	x["message"]="Hello, World!";
	return x;
  });
 // app.route("/fortune")([] {
	//return "Hello, World!";
 // });
  app.loglevel(LogLevel::WARNING).port(8080).multithreaded().run();
}