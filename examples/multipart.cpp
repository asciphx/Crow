#include "cc.h"
int main() {
  cc::App<> app;
  app.route("/upload").methods(cc::HTTP::POST)([](const cc::Req& req) {
	cc::Parser<1024> msg(req, "wwzzgg");
	json j = json::object();
	for (auto p : msg.params) {
	  if (!p.size) j[p.key] = p.value; else j[p.key] = p.filename;
	}
	return j;
	});
  app.port(8080).run();
}