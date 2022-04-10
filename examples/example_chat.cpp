#include "cc.h"
#include <string>
#include <vector>
#include <chrono>

using namespace std;

vector<string> msgs;
vector<pair<cc::Res*,decltype(chrono::steady_clock::now())>> ress;

void broadcast(const string& msg) {
  msgs.push_back(msg);
  json x;
  x["msgs"][0]=msgs.back();
  x["last"]=msgs.size();
  string body=x.dump();
  for (auto p:ress) {
	auto* res=p.first;
	LOG_DEBUG<<res<<" replied: "<<body;
	res->end(body);
  }
  ress.clear();
}
// To see how it works go on {ip}:8080 but I just got it working with external build (not directly in IDE, I guess a problem with dependency)
int main() {
  cc::App<> app;
  app.directory(".").home("example_chat.html");
  app("/logs")([] {
	LOG_INFO<<"logs requested";
	json x;
	int start=max(0,(int)msgs.size()-100);
	for (int i=start; i<(int)msgs.size(); i++)
	  x["msgs"][i-start]=msgs[i];
	x["last"]=msgs.size();
	LOG_INFO<<"logs completed";
	return x;
  });

  app("/logs/<int>")
	([](const cc::Req& /*req*/,cc::Res& res,int after) {
	LOG_INFO<<"logs with last "<<after;
	if (after<(int)msgs.size()) {
	  json x;
	  for (int i=after; i<(int)msgs.size(); i++)
		x["msgs"][i-after]=msgs[i];
	  x["last"]=msgs.size();

	  res.write(x.dump());
	  res.end();
	} else {
	  vector<pair<cc::Res*,decltype(chrono::steady_clock::now())>> filtered;
	  for (auto p:ress) {
		if (p.first->is_alive()&&chrono::steady_clock::now()-p.second<chrono::seconds(30))
		  filtered.push_back(p);
		else
		  p.first->end();
	  }
	  ress.swap(filtered);
	  ress.push_back({&res, chrono::steady_clock::now()});
	  LOG_DEBUG<<&res<<" stored "<<ress.size();
	}
  });

  app("/send").methods("GET"_mt,"POST"_mt)
	([](const cc::Req& req) {
	LOG_INFO<<"msg from client: "<<req.body;
	broadcast(req.body);
	return "";
  });

  app.set_port(8080)
	//.multithreaded()
	.run();
}
