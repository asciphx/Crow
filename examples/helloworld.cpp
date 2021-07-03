#include "crow.h"
using namespace crow;
int main() {
  SimpleApp app;
  CROW_ROUTE(app,"/multipart").methods(HTTPMethod::POST)
	([](const crow::Req& req) {
	crow::multipart::message msg(req);
	CROW_LOG_INFO<<"body of the first part "<<msg.parts[0].body;
	return "it works!";
  });
  app.port(8080).run();
}