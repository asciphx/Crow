#include "crow.h"
#include "mustache.h"
#include <string>
#include <vector>
#include <chrono>

using namespace std;

vector<string> msgs;
vector<pair<crow::Res*,decltype(chrono::steady_clock::now())>> ress;

void broadcast(const string& msg) {
  msgs.push_back(msg);
  crow::json x;
  x["msgs"][0]=msgs.back();
  x["last"]=msgs.size();
  string body=x.dump();
  for (auto p:ress) {
	auto* res=p.first;
	CROW_LOG_DEBUG<<res<<" replied: "<<body;
	res->end(body);
  }
  ress.clear();
}
// To see how it works go on {ip}:40080 but I just got it working with external build (not directly in IDE, I guess a problem with dependency)
int main() {
  crow::SimpleApp app;
  app.set_directory(".").set_home_page("example_chat.html");
  CROW_ROUTE(app,"/logs")([] {
	CROW_LOG_INFO<<"logs requested";
	crow::json x;
	int start=max(0,(int)msgs.size()-100);
	for (int i=start; i<(int)msgs.size(); i++)
	  x["msgs"][i-start]=msgs[i];
	x["last"]=msgs.size();
	CROW_LOG_INFO<<"logs completed";
	return x;
  });

  CROW_ROUTE(app,"/logs/<int>")
	([](const crow::Req& /*req*/,crow::Res& res,int after) {
	CROW_LOG_INFO<<"logs with last "<<after;
	if (after<(int)msgs.size()) {
	  crow::json x;
	  for (int i=after; i<(int)msgs.size(); i++)
		x["msgs"][i-after]=msgs[i];
	  x["last"]=msgs.size();

	  res.write(x.dump());
	  res.end();
	} else {
	  vector<pair<crow::Res*,decltype(chrono::steady_clock::now())>> filtered;
	  for (auto p:ress) {
		if (p.first->is_alive()&&chrono::steady_clock::now()-p.second<chrono::seconds(30))
		  filtered.push_back(p);
		else
		  p.first->end();
	  }
	  ress.swap(filtered);
	  ress.push_back({&res, chrono::steady_clock::now()});
	  CROW_LOG_DEBUG<<&res<<" stored "<<ress.size();
	}
  });

  CROW_ROUTE(app,"/send").methods("GET"_mt,"POST"_mt)
	([](const crow::Req& req) {
	CROW_LOG_INFO<<"msg from client: "<<req.body;
	broadcast(req.body);
	return "";
  });

  app.port(8080)
	//.multithreaded()
	.run();
}
