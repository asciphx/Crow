#include "crow.h"
using namespace crow;
int main() {
  SimpleApp app;
  CROW_ROUTE(app,"/")([]() {
	return "Hello world!";
  });
  json x;
  x["age"]=23;
  x["name"]="Hello, World!";
  app.route_dynamic("/json")([&x] {
	return x;
  });
  app.route_dynamic("/1")([] {
	return "haha";
  });
  CROW_ROUTE(app,"/large")([] {
	return std::string(512*1024,' ');
  });
  // Take a multipart/form-data Req and print out its body
  CROW_ROUTE(app,"/multipart").methods(HTTPMethod::POST)
	([](const crow::Req& req) {
	crow::multipart::message msg(req);
	CROW_LOG_INFO<<"body of the first part "<<msg.parts[0].body;
	return "it works!";
  });
  logger::setLogLevel(LogLevel::DEBUG);
  app.port(8080).run();
}