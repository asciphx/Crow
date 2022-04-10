#include <cc.h>
int main() {
  cc::App<> app;
  //Setting a custom route for any URL that isn't defined, instead of a simple 404.
  CATCHALL_ROUTE(app)
	([](cc::Res& res) {
	res.body="The URL does not seem to be correct.";
	res.end();
  });

  app.set_port(8080).run();
}
