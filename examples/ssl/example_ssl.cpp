#define ENABLE_SSL
#include "cc.h"
int main() {
  cc::App<> app;
  app.port(8080).ssl_file("test.crt","test.key").run();
  // Use .pem file
  //app.port(8080).ssl_file("test.pem").run();
  // Use custom context; see boost::asio::ssl::context
  /*
   * cc::ssl_context_t ctx;
   * ctx.set_verify_mode(...)
   *
   *   ... configuring ctx
   *
   *   app.port(18080).ssl(ctx).run();
   */
}
