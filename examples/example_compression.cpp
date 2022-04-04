#define ENABLE_COMPRESSION
#include "cc.h"
#include "cc/compression.h"

int main() {
  cc::App<> app;
  //cc::App<cc::CompressionGzip> app;
  ROUTE(app,"/hello")([&](const cc::Req&,cc::Res& res) {
	res.compressed=false;
	res.body="Hello World! This is uncompressed!";
	res.end();
  });

  ROUTE(app,"/hello_compressed")([]() {
	return "Hello World! This is compressed by default!";
  });
  app.port(8080)
	.use_compression(cc::compression::algorithm::DEFLATE)
	//.use_compression(cc::compression::algorithm::GZIP)
	.loglevel(cc::LogLevel::DEBUG)
	.multithreaded()
	.run();
}
