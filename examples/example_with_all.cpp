#include "../amalgamate/crow_all.h"
#include <sstream>
int main() {
  crow::App<> app;
  CROW_ROUTE(app,"/about")([]() {
	return "About Crow example.";
  });
  CROW_CATCHALL_ROUTE(app)([] {
	return "The URL does not seem to be correct.";
  });
  // simple json response
  CROW_ROUTE(app,"/json")([] {
	json x;
	x["message"]="Hello, World!";
	return x;
  });
  CROW_ROUTE(app,"/hello/<int>")([](int count) {
	if (count>100)
	  return crow::Res(400);
	std::ostringstream os;
	os<<count<<" bottles of beer!";
	return crow::Res(os.str());
  });
  CROW_ROUTE(app,"/add/<int>/<int>")([](const crow::Req&,crow::Res& res,int a,int b) {
	std::ostringstream os;
	os<<a+b;
	res.write(os.str());
	res.end();
  });
  // Compile error with message "Handler type is mismatched with URL paramters"
  //CROW_ROUTE(app,"/another/<int>")
  //([](int a, int b){
	  //return crow::Res(500);
  //});
  CROW_ROUTE(app,"/add_json").methods("POST"_mt)([](const crow::Req& req) {
	auto x=json::parse(req.body);
	if (!x) return crow::Res(400);
	int sum=x["a"].get<int>()+x["b"].get<int>();
	std::ostringstream os;
	os<<sum;
	return crow::Res{os.str()};
  });
  CROW_ROUTE(app,"/params")
	([](const crow::Req& req) {
	std::ostringstream os;
	os<<"Params: "<<req.url_params<<"\n\n";
	os<<"The key 'foo' was "<<(req.url_params.get("foo")==nullptr?"not ":"")<<"found.\n";
	if (req.url_params.get("pew")!=nullptr) {
	  double countD=boost::lexical_cast<double>(req.url_params.get("pew"));
	  os<<"The value of 'pew' is "<<countD<<'\n';
	}
	auto count=req.url_params.get_list("count");
	os<<"The key 'count' contains "<<count.size()<<" value(s).\n";
	for (const auto& countVal:count) {
	  os<<" - "<<countVal<<'\n';
	}
	return crow::Res{os.str()};
  });
  // ignore all log
  crow::logger::setLogLevel(crow::LogLevel::DEBUG);
  //crow::logger::setHandler(std::make_shared<ExampleLogHandler>());
  app.port(8080)
	.multithreaded()
	.run();
}
