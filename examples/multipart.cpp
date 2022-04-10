#include "cc.h"
int main() {
  cc::App<> app;
  app.post("/upload")([](const cc::Req& req) {
	return cc::Parser<1024>(req);
	});
  app.set_port(8080).run();
}