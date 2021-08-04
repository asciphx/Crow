#include "crow.h"
#include "middleware.h"
#include "module.h"
#include <sstream>
using namespace crow; auto d = D_();
//auto d = D_sqlite("test.db");
class ExampleLogHandler : public ILogHandler {
  public:void log(std::string message,LogLevel /*level*/) override {
	std::cerr << "ExampleLogHandler -> " <<message;
  }
};
int main() {
  App<ExampleMiddleware/*,Middle*/> app;//Global Middleware,and default config
  app.directory("./static").home("i.htm").timeout(2)
	.file_type({"html","ico","css","js","json","svg","png","jpg","gif","txt"})
	.get_middleware<ExampleMiddleware>().setMessage("hello");
  //Server rendering and support default route
  app.default_route()([] {
	char name[64];gethostname(name,64);
	json j{{"servername",name}};
	return mustache::load("404NotFound.html").render(j);
  });
  //sql
  app.route("/sql")([] {
	auto q = d.conn();
	//std::tuple<int, std::string> ds=q("select id,name from users_test where id = 1").template r__<int,std::string>();
	//std::cout<<std::get<0>(ds)<<std::get<1>(ds);
	int i = 0; q("SELECT 200+2").r__(i);
	std::string s; q("SELECT 'hello world'").r__(s);
	return Res(i, s);
  });
  //json::parse
  app.route("/list")([] {
	json v=json::parse(R"({"user":{"is":false,"age":25,"weight":50.6,"name":"deaod"},
	  "userList":[{"is":true,"weight":52.0,"age":23,"state":true,"name":"wwzzgg"},
	  {"is":true,"weight":51.0,"name":"best","age":26}]})");
	return v;
  });
  //static reflect
  app.route("/lists")([] {
	List list=json::parse(R"({"user":{"is":false,"age":25,"weight":50.6,"name":"deaod"},
	  "userList":[{"is":true,"weight":52.0,"age":23,"state":true,"name":"wwzzgg"},
	  {"is":true,"weight":51.0,"name":"best","age":26}]})").get<List>();
	json json_output=json(list);
	return json_output;
  });
  //status code + return json
  app.route("/json")([] {
	json x;
	x["message"]="Hello, World!";
	x["double"]=3.1415926;
	x["int"]=2352352;
	x["true"]=true;
	x["false"]=false;
	x["null"]=nullptr;
	x["bignumber"]=2353464586543265455;
	return Res(203,x);
  });
  // a request to /path should be forwarded to /path/
  app.route("/path/")([] {
	return "Trailing slash test case..";
  });
  // To see it in action enter {ip}:18080/hello/{integer_between -2^32 and 100} and you should receive
  // {integer_between -2^31 and 100} bottles of beer!
  app.route("/hello/<int>")([](int count) {
	if (count>100)
	  return crow::Res(400);
	std::ostringstream os;
	os<<count<<" bottles of beer!";
	return crow::Res(os.str());
  });
  // To see it in action submit {ip}:18080/add/1/2 and you should receive 3 (exciting, isn't it)
  app.route("/add/<int>/<int>")([](const Req& req,Res& res,int a,int b) {
	std::ostringstream os;
	os<<a+b;
	res.write(os.str());
	res.end();
  });
  // Compile error with message "Handler type is mismatched with URL paramters"
  //CROW_ROUTE(app,"/another/<int>")
  //([](int a, int b){
	  //return crow::response(500);
  //});
  // more json example
  // To see it in action, I recommend to use the Postman Chrome extension:
  //      * Set the address to {ip}:18080/add_json
  //      * Set the method to post
  //      * Select 'raw' and then JSON
  //      * Add {"a": 1, "b": 1}
  //      * Send and you should receive 2
  // A simpler way for json example:
  //      * curl -d '{"a":1,"b":2}' {ip}:18080/add_json
  CROW_ROUTE(app,"/add_json")
	.methods("POST"_mt)
	([](const crow::Req& req) {
	auto x=crow::json::parse(req.body);
	if (!x)
	  return crow::Res(400);
	int sum=x["a"].get<int>()+x["b"].get<int>();
	std::ostringstream os;
	os<<sum;
	return crow::Res{os.str()};
  });
  // Example of a request taking URL parameters
  // If you want to activate all the functions just query
  // {ip}:18080/params?foo='blabla'&pew=32&count[]=a&count[]=b
  CROW_ROUTE(app,"/params")
	([](const crow::Req& req) {
	std::ostringstream os;
	// To get a simple string from the url params
	// To see it in action /params?foo='blabla'
	os<<"Params: "<<req.url_params<<"\n\n";
	os<<"The key 'foo' was "<<(req.url_params.get("foo")==nullptr?"not ":"")<<"found.\n";
	// To get a double from the request
	// To see in action submit something like '/params?pew=42'
	if (req.url_params.get("pew")!=nullptr) {
	  double countD=boost::lexical_cast<double>(req.url_params.get("pew"));
	  os<<"The value of 'pew' is "<<countD<<'\n';
	}
	// To get a list from the request
	// You have to submit something like '/params?count[]=a&count[]=b' to have a list with two values (a and b)
	auto count=req.url_params.get_list("count");
	os<<"The key 'count' contains "<<count.size()<<" value(s).\n";
	for (const auto& countVal:count) {
	  os<<" - "<<countVal<<'\n';
	}
	// To get a dictionary from the request
	// You have to submit something like '/params?mydict[a]=b&mydict[abcd]=42' to have a list of pairs ((a, b) and (abcd, 42))
	auto mydict=req.url_params.get_dict("mydict");
	os<<"The key 'dict' contains "<<mydict.size()<<" value(s).\n";
	for (const auto& mydictVal:mydict) {
	  os<<" - "<<mydictVal.first<<" -> "<<mydictVal.second<<'\n';
	}
	return crow::Res{os.str()};
  });
  CROW_ROUTE(app,"/large")([] {
	return std::string(512*1024,' ');
  });

  //crow::logger::setHandler(std::make_shared<ExampleLogHandler>());
  app.loglevel(LogLevel::WARNING).port(8080).multithreaded().run();
}
