#include "crow.h"
int main() {
  crow::App<> app;
  CROW_ROUTE(app,"/multipart").methods(crow::HTTPMethod::POST)
	([](const crow::Req& req) {
	crow::multipart::message msg(req);
	CROW_LOG_INFO<<"body of the first part "<<msg.parts[0].body;
	return "it works!";
  });
  app.port(8080).run();
}