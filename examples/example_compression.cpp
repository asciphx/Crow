#define ENABLE_COMPRESSION
#include "cc.h"
int main() {
  cc::App<> app;
  app["/hello"]([&](cc::Req&,cc::Res& res) {
	res.compressed=false;
	res.body="Hello World! This is uncompressed!";
	res.end();
  });
  app["/hello_compressed"]([]() {
	return "Hello World! This is compressed by default!";
  });
  app.set_port(8080)
	//.use_compression(cc::compression::algorithm::DEFLATE)
	.use_compression(cc::compression::algorithm::GZIP)
	.loglevel(cc::LogLevel::DEBUG)
	.multithreaded()
	.run();
}
