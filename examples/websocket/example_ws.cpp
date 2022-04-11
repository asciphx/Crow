#define DISABLE_HOME
#include "cc.h"
#include <unordered_set>
#include <mutex>
int main() {
  cc::App<> app;
  std::mutex mtx;
  std::unordered_set<cc::websocket::connection*> users;
  app("/ws")
	.websocket()
	.onopen([&](cc::websocket::connection& conn) {
	std::lock_guard<std::mutex> _(mtx);
	users.insert(&conn);
  })
	.onclose([&](cc::websocket::connection& conn, const std::string& reason) {
	LOG_INFO("websocket connection closed: " << reason);
	std::lock_guard<std::mutex> _(mtx);
	users.erase(&conn);
  })
	.onmessage([&](cc::websocket::connection& /*conn*/, const std::string& data, bool is_binary) {
	std::lock_guard<std::mutex> _(mtx);
	for (auto u : users)
	  if (is_binary)
		u->send_binary(data);
	  else
		u->send_text(data);
  });
  app("/")([] {
	char name[64]; gethostname(name, 64);
	json j{ {"servername",name} };
	return cc::mustache::load("ws.html").render(j);
	});
  app.set_port(8080).multithreaded()
	.run();
}
