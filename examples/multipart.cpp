#include "crow.h"
int main() {
  crow::App<> app;
  app.route("/upload").methods(crow::HTTPMethod::POST)([](const crow::Req& req) {
	crow::Parser<1024> msg(req);
	json j = json::object();
	for (auto p : msg.params) {
	  if (!p.size) j[p.key] = p.value; else j[p.key] = p.filename;
	}
	return j;
	});
  app.port(8080).run();
}