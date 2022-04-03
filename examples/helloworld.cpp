#include "crow.h"
int main() {
  crow::App<> app;
  app.route("/multipart").methods(crow::HTTPMethod::POST)([](const crow::Req& req) {
	crow::Parser<1024> msg(req);
	json j = json::object();
	for (auto p : msg.params) {
	  if (!p.size) j[p.key] = p.value;// else std::cout << p.key << ": " << p.filename << " , " << p.size << ";\n";
	}
	return j;
	});
  app.port(8080).run();
}