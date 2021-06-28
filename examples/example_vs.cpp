#include "crow.h"
#include "mustache.h"
#include "middleware.h"
#include "module.h"
#include <sstream>
using namespace crow;
int main() {
  App<Cors> app;//Global Middleware,and default config
  app.set_directory("./static")
	.set_types({"html","ico","css","js","json","svg","png","jpg","gif","txt"});
  //Server rendering
  CROW_ROUTE(app,"/")([] {
	char name[64];gethostname(name,64);
	mustache::Ctx x;x["servername"]=name;
	auto page=mustache::load("index.html");
	return page.render(x);
  });
  //Single path access to files
  app.route_dynamic("/cat")([](const Req&,Res& res) {
	res.set_static_file_info("1.jpg");res.end();
  });
  // a request to /path should be forwarded to /path/
  app.route_dynamic("/path/")([]() {
	return "Trailing slash test case..";
  });
  app.route_dynamic("/list")([]() {
	List list=json::parse(R"({"user":{"is":false,"age":25,"weight":50.6,"name":"www","state":null},"userList":[{"is":true,"weight":52.0,"age":23,"state":true,"name":"wwzzgg"},
	{"is":true,"weight":51.0,"name":"best","age":26}]})").get<List>();
	json json_output=json(list);
	return json_output.dump(2);
  });
  app.route_dynamic("/json")([] {
	json x;
	x["message"]="Hello, World!";
	x["double"]=3.1415926;
	x["int"]=2352352;
	x["true"]=true;
	x["false"]=false;
	x["null"]=nullptr;
	x["bignumber"]=2353464586543265455;
	return x;
  });
  app.route_dynamic("/hello/<int>")([](int count) {
	if (count>100)
	  return Res(400);
	std::ostringstream os;
	os<<count<<" bottles of beer!";
	return Res(os.str());
  });
  app.route_dynamic("/add/<int>/<int>")([](const Req& req,Res& res,int a,int b) {
	std::ostringstream os;
	os<<a+b;
	res.write(os.str());
	res.end();
  });
  // Compile error with message "Handler type is mismatched with URL paramters"
  //CROW_ROUTE(app,"/another/<int>")([](int a, int b){
	  //return response(500);
  //});
  // more json example
  app.route_dynamic("/parse_json").methods(HTTPMethod::POST)([](const Req& req) {
	auto x=json::parse(req.body);
	if (!x) return Res(400);
	std::ostringstream os; os<<x;
	return Res{os.str()};
  });
  app.route_dynamic("/params")([](const Req& req) {
	std::ostringstream os;
	os<<"Params: "<<req.url_params<<"\n\n";
	os<<"The key 'foo' was "<<(req.url_params.get("foo")==nullptr?"not ":"")<<"found.\n";
	if (req.url_params.get("pew")!=nullptr) {
	  double countD=boost::lexical_cast<double>(req.url_params.get("pew"));
	  os<<"The value of 'pew' is "<<countD<<'\n';
	}
	auto count=req.url_params.get_list("count");
	os<<"The key 'count' contains "<<count.size()<<" value(s).\n";
	for (const auto& countVal:count) os<<" - "<<countVal<<'\n';
	return Res{os.str()};
  });
  CROW_ROUTE(app,"/large")([] {
	return std::string(512*1024,' ');
  });
  // Take a multipart/form-data Req and print out its body
  CROW_ROUTE(app,"/multipart")([](const crow::Req& req) {
	crow::multipart::message msg(req);
	CROW_LOG_INFO<<"body of the first part "<<msg.parts[0].body;
	return "it works!";
  });
  logger::setLogLevel(LogLevel::WARNING);
  //logger::setHandler(std::make_shared<ExampleLogHandler>());
  app.port(8080).multithreaded().run();
}