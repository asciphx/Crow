#define ENABLE_COMPRESSION
#include "cc.h"
#include "middleware.h"
#include "module.h"
using namespace cc; auto d = D_();//auto d = D_pgsql();
//auto d = D_sqlite("test.db");
int main() {
  App</*Middle*/> app;//Global Middleware,and default config
  app.directory("static").home("i.htm").timeout(3).upload_path("upload")
    .file_type({ "html","ico","css","js","json","svg","png","gif","jpg","txt" });
  //Server rendering and support default route
  app.default_route()([] {
    char name[64]; gethostname(name, 64);
    json j{ {"servername",name} };
    return mustache::load("404NotFound.html").render(j);
    });
  //sql
  app["/sql"]([] {
    auto q = d.conn();
    //json v = q("select * from user where id = 1").JSON(); std::cout << v;
    int i = 200; q("SELECT 200+2").r__(i);
    std::string s; q("SELECT '你好 世界！'").r__(s);
    return Res(i, s);
    });
  // a request to /path should be forwarded to /path/
  app["/path/"]([]() { return "Trailing slash test case.."; });
  // upload file
  app.post("/upload")([](Req& req) { return Parser<4096>(req); });
  //static reflect
  app["/lists"]([]() {
        Tab t{1, true, "ref1", now(), Type{1, "model1", 3.141593}}; // Up to 2 layers
        json::parse(t, R"(
{
  "id": 3,
  "ok": false,
  "name": "🍻🍺!",
  "date": "2021-09-08 01:04:30",
  "type":  {
    "bigBlob": 0.1,
    "id": 4,
    "language": "元宇宙🌀🌌🪐!",
    "tab":  {
      "id": 5,
      "name": "!!!!!",
      "type":  {
        "bigBlob": 0.2,
        "id": 6,
        "language": "元宇宙2🌀🌌🪐!",
        "tab":  {
          "id": 7,
          "name": "!!!!!!!!",
          "type":  {
            "bigBlob": 0.3,
            "id": 8,
            "language": "元宇宙4🌀🌌🪐!",
            "tab":  {
              "id": 9,
              "name": "!!!!!!!!!!"
            }
          }
        }
      }
    }
  }
}
)");
    return json(t);
    });
  //status code + return json
  app["/json"]([] {
    json x;
    x["message"] = "你好 世界！";
    x["double"] = 3.1415926;
    x["int"] = 2352352;
    x["true"] = true;
    x["false"] = false;
    x["null"] = nullptr;
    x["bignumber"] = 2353464586543265455;
    return Res(202, x);
    });
  //json::parse
  app["/list"]([]() {
    List list; json::parse(list, R"({"user":{"is":false,"age":25,"weight":50.6,"name":"deaod"},
	  "userList":[{"is":true,"weight":52.0,"age":23,"state":true,"name":"wwzzgg"},
	  {"is":true,"weight":51.0,"name":"best","age":26}]})");
    return json(list);
    });
  //ostringstream
  app["/hello/<int>"]([](int count) {
    if (count > 100) return Res(400);
    std::ostringstream os;
    os << count << " bottles of beer!";
    return Res(203, os.str());
    });
  //rank routing
  app["/add/<int>/<int>"]([](Req& req, Res& res, int a, int b) {
    std::ostringstream os;
    os << a + b;
    res.write(os.str());
    res.end();
    });
  // Compile error with message "Handler type is mismatched with URL paramters"
  //app["/another/<int>"]([](int a, int b){
    //return response(500);
  //});
  // more json example
  app.post("/add_json")([](Req& req) {
    auto x = json::parse(req.body);
    if (!x) return Res(400);
    int sum = x["a"].get<int>() + x["b"].get<int>();
    std::ostringstream os; os << sum;
    return Res{ os.str() };
    });
  app["/params"]([](Req& req) {
    std::ostringstream os;
    os << "Params: " << req.url_params << "\n\n";
    os << "The key 'foo' was " << (req.url_params.get("foo") == nullptr ? "not " : "") << "found.\n";
    if (req.url_params.get("pew") != nullptr) {
      double countD = boost::lexical_cast<double>(req.url_params.get("pew"));
      os << "The value of 'pew' is " << countD << '\n';
    }
    auto count = req.url_params.get_list("count");
    os << "The key 'count' contains " << count.size() << " value(s).\n";
    for (const auto& countVal : count) os << " - " << countVal << '\n';
    return Res{ os.str() };
    });
  //logger::setHandler(std::make_shared<ExampleLogHandler>());
  app.loglevel(LogLevel::WARNING).set_port(8080).multithreaded().run();
}