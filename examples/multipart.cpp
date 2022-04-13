#define DEFAULT_ENABLE_LOGGING 1//open logger
#define ENABLE_COMPRESSION
#include "cc.h"
int main() {
  cc::App<> app;
  app.post("/upload")([](cc::Req& req) {
	return cc::Parser<1024>(req);
	});
  app.set_port(8080).run();
}