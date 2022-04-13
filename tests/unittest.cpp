#include <iostream>
#include <sstream>
#include <vector>
#include "cc.h"
#ifdef DEFAULT_LOG_LEVEL
#undef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 0
#endif
#include "middleware.h"

using namespace std;
using namespace cc;

struct Test { Test(); virtual void test()=0; };
vector<Test*> tests;
Test::Test() { tests.push_back(this); }

bool failed__=false;
void error_print() {
  cerr<<endl;
}

template <typename A,typename ...Args>
void error_print(const A& a,Args...args) {
  cerr<<a;
  error_print(args...);
}

template <typename ...Args>
void fail(Args...args) { error_print(args...);failed__=true; }

#define ASSERT_TRUE(x) if (!(x)) fail(__FILE__ ":", __LINE__, ": Assert fail: expected ", #x, " is true, at " __FILE__ ":",__LINE__)
#define ASSERT_EQUAL(a, b) if ((a) != (b)) fail(__FILE__ ":", __LINE__, ": Assert fail: expected ", (a), " actual ", (b),  ", " #a " == " #b ", at " __FILE__ ":",__LINE__)
#define ASSERT_NOTEQUAL(a, b) if ((a) == (b)) fail(__FILE__ ":", __LINE__, ": Assert fail: not expected ", (a), ", " #a " != " #b ", at " __FILE__ ":",__LINE__)
#define ASSERT_THROW(x) \
    try \
    { \
        x; \
        fail(__FILE__ ":", __LINE__, ": Assert fail: exception should be thrown"); \
    } \
    catch(std::exception&) \
    { \
    }



#define TEST(x) struct test##x:public Test{void test();}x##_; \
    void test##x::test()
#define DISABLE_TEST(x) struct test##x{void test();}x##_; \
    void test##x::test()


#define LOCALHOST_ADDRESS "127.0.0.1"


TEST(Rule) {
  TaggedRule<> r("/http/");
  r.name("abc");

  // empty handler - fail to validate
  try {
	r.validate();
	fail("empty handler should fail to validate");
  } catch (runtime_error& e) {
  }

  int x=0;

  // registering handler
  r([&x] {x=1;return "";});

  r.validate();

  Res res;

  // executing handler
  ASSERT_EQUAL(0,x);
  r.handle(Req(),res,routing_params());
  ASSERT_EQUAL(1,x);

  // registering handler with request argument
  r([&x](cc::Req&) {x=2;return "";});

  r.validate();

  // executing handler
  ASSERT_EQUAL(1,x);
  r.handle(Req(),res,routing_params());
  ASSERT_EQUAL(2,x);
}

TEST(ParameterTagging) {
  static_assert(spell::is_valid("<int><int><int>"),"valid url");
  static_assert(!spell::is_valid("<int><int<<int>"),"invalid url");
  static_assert(!spell::is_valid("nt>"),"invalid url");
  ASSERT_EQUAL(1,spell::get_parameter_tag("<int>"));
  ASSERT_EQUAL(2,spell::get_parameter_tag("<uint>"));
  ASSERT_EQUAL(3,spell::get_parameter_tag("<float>"));
  ASSERT_EQUAL(3,spell::get_parameter_tag("<double>"));
  ASSERT_EQUAL(4,spell::get_parameter_tag("<str>"));
  ASSERT_EQUAL(4,spell::get_parameter_tag("<string>"));
  ASSERT_EQUAL(5,spell::get_parameter_tag("<path>"));
  ASSERT_EQUAL(6*6+6+1,spell::get_parameter_tag("<int><int><int>"));
  ASSERT_EQUAL(6*6+6+2,spell::get_parameter_tag("<uint><int><int>"));
  ASSERT_EQUAL(6*6+6*3+2,spell::get_parameter_tag("<uint><double><int>"));

  // url definition parsed in compile time, build into *one number*, and given to template argument
  static_assert(std::is_same<spell::S<uint64_t,double,int64_t>,spell::arguments<6*6+6*3+2>::type>::value,"tag to type container");
}

TEST(PathRouting) {
  App<> app;

  ROUTE(app,"/file")
	([] {
	return "file";
  });

  ROUTE(app,"/path/")
	([] {
	return "path";
  });

  app.validate();

  {
	Req req;
	Res res;

	req.url="/file";

	app.handle(req,res);

	ASSERT_EQUAL(200,res.code);
  }
  {
	Req req;
	Res res;

	req.url="/file/";

	app.handle(req,res);
	ASSERT_EQUAL(404,res.code);
  }
  {
	Req req;
	Res res;

	req.url="/path";

	app.handle(req,res);
	ASSERT_NOTEQUAL(404,res.code);
  }
  {
	Req req;
	Res res;

	req.url="/path/";

	app.handle(req,res);
	ASSERT_EQUAL(200,res.code);
  }
}

TEST(RoutingTest) {
  App<> app;
  int A{};
  uint32_t B{};
  double C{};
  string D{};
  string E{};

  ROUTE(app,"/0/<uint>")
	([&](uint32_t b) {
	B=b;
	return "OK";
  });

  ROUTE(app,"/1/<int>/<uint>")
	([&](int a,uint32_t b) {
	A=a; B=b;
	return "OK";
  });

  ROUTE(app,"/4/<int>/<uint>/<double>/<string>")
	([&](int a,uint32_t b,double c,string d) {
	A=a; B=b; C=c; D=d;
	return "OK";
  });

  ROUTE(app,"/5/<int>/<uint>/<double>/<string>/<path>")
	([&](int a,uint32_t b,double c,string d,string e) {
	A=a; B=b; C=c; D=d; E=e;
	return "OK";
  });

  app.validate();
  //app.debug_print();
  {
	Req req;
	Res res;

	req.url="/-1";

	app.handle(req,res);

	ASSERT_EQUAL(404,res.code);
  }

  {
	Req req;
	Res res;

	req.url="/0/1001999";

	app.handle(req,res);

	ASSERT_EQUAL(200,res.code);

	ASSERT_EQUAL(1001999,B);
  }

  {
	Req req;
	Res res;

	req.url="/1/-100/1999";

	app.handle(req,res);

	ASSERT_EQUAL(200,res.code);

	ASSERT_EQUAL(-100,A);
	ASSERT_EQUAL(1999,B);
  }
  {
	Req req;
	Res res;

	req.url="/4/5000/3/-2.71828/hellhere";
	req.add_header("TestHeader","Value");

	app.handle(req,res);

	ASSERT_EQUAL(200,res.code);

	ASSERT_EQUAL(5000,A);
	ASSERT_EQUAL(3,B);
	ASSERT_EQUAL(-2.71828,C);
	ASSERT_EQUAL("hellhere",D);
  }
  {
	Req req;
	Res res;

	req.url="/5/-5/999/3.141592/hello_there/a/b/c/d";
	req.add_header("TestHeader","Value");

	app.handle(req,res);

	ASSERT_EQUAL(200,res.code);

	ASSERT_EQUAL(-5,A);
	ASSERT_EQUAL(999,B);
	ASSERT_EQUAL(3.141592,C);
	ASSERT_EQUAL("hello_there",D);
	ASSERT_EQUAL("a/b/c/d",E);
  }
}

TEST(simple_response_routing_params) {
  ASSERT_EQUAL(100,Res(100).code);
  //ASSERT_EQUAL(200,Res("Hello there").code);
  //ASSERT_EQUAL(500,Res(500,"Internal Error?").code);

  routing_params rp;
  rp.int_params.push_back(1);
  rp.int_params.push_back(5);
  rp.uint_params.push_back(2);
  rp.double_params.push_back(3);
  rp.string_params.push_back("hello");
  ASSERT_EQUAL(1,rp.get<int64_t>(0));
  ASSERT_EQUAL(5,rp.get<int64_t>(1));
  ASSERT_EQUAL(2,rp.get<uint64_t>(0));
  ASSERT_EQUAL(3,rp.get<double>(0));
  ASSERT_EQUAL("hello",rp.get<string>(0));
}

TEST(handler_with_response) {
  App<> app;
  ROUTE(app,"/")([](cc::Req&,cc::Res&) {
  });
}

TEST(http_method) {
  App<> app;

  ROUTE(app,"/")
	.methods("POST"_mt,"GET"_mt)
	([](Req& req) {
	if (req.method=="GET"_mt)
	  return "2";
	else
	  return "1";
  });

  ROUTE(app,"/get_only")
	.methods("GET"_mt)
	([](Req& /*req*/) {
	return "get";
  });
  ROUTE(app,"/post_only")
	.methods("POST"_mt)
	([](Req& /*req*/) {
	return "post";
  });
  ROUTE(app,"/patch_only")
	.methods("PATCH"_mt)
	([](Req& /*req*/) {
	return "patch";
  });
  ROUTE(app,"/purge_only")
	.methods("PURGE"_mt)
	([](Req& /*req*/) {
	return "purge";
  });

  app.validate();
  app.debug_print();


  // cannot have multiple handlers for the same url
  //ROUTE(app, "/")
  //.methods("GET"_mt)
  //([]{ return "2"; });

  {
	Req req;
	Res res;

	req.url="/";
	app.handle(req,res);

	ASSERT_EQUAL("2",res.body);
  }
  {
	Req req;
	Res res;

	req.url="/";
	req.method="POST"_mt;
	app.handle(req,res);

	ASSERT_EQUAL("1",res.body);
  }

  {
	Req req;
	Res res;

	req.url="/get_only";
	app.handle(req,res);

	ASSERT_EQUAL("get",res.body);
  }

  {
	Req req;
	Res res;

	req.url="/patch_only";
	req.method="PATCH"_mt;
	app.handle(req,res);

	ASSERT_EQUAL("patch",res.body);
  }

  {
	Req req;
	Res res;

	req.url="/purge_only";
	req.method="PURGE"_mt;
	app.handle(req,res);

	ASSERT_EQUAL("purge",res.body);
  }

  {
	Req req;
	Res res;

	req.url="/get_only";
	req.method="POST"_mt;
	app.handle(req,res);

	ASSERT_NOTEQUAL("get",res.body);
  }

}

TEST(server_handling_error_request) {
  static char buf[2048];
  App<> app;
  ROUTE(app,"/")([] {return "A";});
  //Server<App<>> server(&app, LOCALHOST_ADDRESS, 45451);
  //auto _ = async(launch::async, [&]{server.run();});
  auto _=async(launch::async,[&] {app.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  app.wait_for_server_start();
  std::string sendmsg="POX";
  asio::io_service is;
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));


	c.send(asio::buffer(sendmsg));

	try {
	  c.receive(asio::buffer(buf,2048));
	  fail();
	} catch (std::exception& e) {
	  //std::cerr << e.what() << std::endl;
	}
  }
  app.stop();
}

TEST(multi_server) {
  static char buf[2048];
  App<> app1,app2;
  ROUTE(app1,"/").methods("GET"_mt,"POST"_mt)([] {return "A";});
  ROUTE(app2,"/").methods("GET"_mt,"POST"_mt)([] {return "B";});

  //Server<App<>> server1(&app1, LOCALHOST_ADDRESS, 45451);
  //Server<App<>> server2(&app2, LOCALHOST_ADDRESS, 45452);

  //auto _ = async(launch::async, [&]{server1.run();});
  //auto _2 = async(launch::async, [&]{server2.run();});
  auto _=async(launch::async,[&] {app1.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  auto _2=async(launch::async,[&] {app2.bindaddr(LOCALHOST_ADDRESS).set_port(45452).run();});
  app1.wait_for_server_start();
  app2.wait_for_server_start();

  std::string sendmsg="POST /\r\nContent-Length:3\r\nX-HeaderTest: 123\r\n\r\nA=B\r\n";
  asio::io_service is;
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));

	c.send(asio::buffer(sendmsg));

	size_t recved=c.receive(asio::buffer(buf,2048));
	ASSERT_EQUAL('A',buf[recved-1]);
  }

  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45452));

	for (auto ch:sendmsg) {
	  char buf[1]={ch};
	  c.send(asio::buffer(buf));
	}

	size_t recved=c.receive(asio::buffer(buf,2048));
	ASSERT_EQUAL('B',buf[recved-1]);
  }

  app1.stop();
  app2.stop();
}

TEST(json_read) {
  {
	const char* json_error_tests[]=
	{
		"{} 3", "{{}", "{3}",
		"3.4.5", "+3", "3-2", "00", "03", "1e3e3", "1e+.3",
		"nll", "f", "t",
		"{\"x\":3,}",
		"{\"x\"}",
		"{\"x\":3   q}",
		"{\"x\":[3 4]}",
		"{\"x\":[\"",
		"{\"x\":[[], 4],\"y\",}",
		"{\"x\":[3",
		"{\"x\":[ null, false, true}",
	};
	for (auto s:json_error_tests) {
	  auto x=json::parse(s);
	  if (x) {
		fail("should fail to parse ",s);
		return;
	  }
	}
  }

  auto x=json::parse(R"({"message":"hello, world"})");
  if (!x)
	fail("fail to parse");
  ASSERT_EQUAL("hello, world",x["message"]);
  ASSERT_EQUAL(1,x.size());
  ASSERT_EQUAL(false,x.at("mess"));
  ASSERT_THROW(x["mess"]);
  // TODO returning false is better than exception
  //ASSERT_THROW(3 == x["message"]);
  ASSERT_EQUAL(12,x["message"].size());

  std::string s=R"({"int":3,     "ints"  :[1,2,3,4,5],	"bigint":1234567890	})";
  auto y=json::parse(s);
  ASSERT_EQUAL(3,y["int"]);
  ASSERT_EQUAL(3.0,y["int"]);
  ASSERT_NOTEQUAL(3.01,y["int"]);
  ASSERT_EQUAL(5,y["ints"].size());
  ASSERT_EQUAL(1,y["ints"][0]);
  ASSERT_EQUAL(2,y["ints"][1]);
  ASSERT_EQUAL(3,y["ints"][2]);
  ASSERT_EQUAL(4,y["ints"][3]);
  ASSERT_EQUAL(5,y["ints"][4]);
  ASSERT_EQUAL(1u,y["ints"][0]);
  ASSERT_EQUAL(1.f,y["ints"][0]);

  std::string s2=R"({"bools":[true, false], "doubles":[1.2, -3.4]})";
  auto z=json::parse(s2);
  ASSERT_EQUAL(2,z["bools"].size());
  ASSERT_EQUAL(2,z["doubles"].size());
  ASSERT_EQUAL(true,z["bools"][0].get<bool>());
  ASSERT_EQUAL(false,z["bools"][1].get<bool>());
  ASSERT_EQUAL(1.2,z["doubles"][0].get<double>());
  ASSERT_EQUAL(-3.4,z["doubles"][1].get<double>());

  std::string s3=R"({"uint64": 18446744073709551615})";
  auto z1=json::parse(s3);
  ASSERT_EQUAL(18446744073709551615ull,z1["uint64"].get<unsigned long long>());

  std::ostringstream os;
  os<<z1["uint64"];
  ASSERT_EQUAL("18446744073709551615",os.str());
}

TEST(json_read_real) {
  vector<std::string> v{"0.036303908355795146", "0.18320417789757412",
  "0.05319940476190476", "0.15224702380952382", "0", "0.3296201145552561",
  "0.47921580188679247", "0.05873511904761905", "0.1577827380952381",
  "0.4996841307277628", "0.6425412735849056", "0.052113095238095236",
  "0.12830357142857143", "0.7871041105121294", "0.954220013477089",
  "0.05869047619047619", "0.1625", "0.8144794474393531",
  "0.9721613881401617", "0.1399404761904762", "0.24470238095238095",
  "0.04527459568733154", "0.2096950808625337", "0.35267857142857145",
  "0.42791666666666667", "0.855731974393531", "0.9352467991913747",
  "0.3816220238095238", "0.4282886904761905", "0.39414167789757415",
  "0.5316079851752021", "0.3809375", "0.4571279761904762",
  "0.03522995283018868", "0.1915641846361186", "0.6164136904761904",
  "0.7192708333333333", "0.05675117924528302", "0.21308541105121293",
  "0.7045386904761904", "0.8016815476190476"};
  for (auto x:v) {
	LOG_DEBUG(x);
	ASSERT_EQUAL(json::parse(x).get<double>(),boost::lexical_cast<double>(x));
  }

  auto ret=json::parse(R"---({"balloons":[{"mode":"ellipse","left":0.036303908355795146,"right":0.18320417789757412,"top":0.05319940476190476,"bottom":0.15224702380952382,"index":"0"},{"mode":"ellipse","left":0.3296201145552561,"right":0.47921580188679247,"top":0.05873511904761905,"bottom":0.1577827380952381,"index":"1"},{"mode":"ellipse","left":0.4996841307277628,"right":0.6425412735849056,"top":0.052113095238095236,"bottom":0.12830357142857143,"index":"2"},{"mode":"ellipse","left":0.7871041105121294,"right":0.954220013477089,"top":0.05869047619047619,"bottom":0.1625,"index":"3"},{"mode":"ellipse","left":0.8144794474393531,"right":0.9721613881401617,"top":0.1399404761904762,"bottom":0.24470238095238095,"index":"4"},{"mode":"ellipse","left":0.04527459568733154,"right":0.2096950808625337,"top":0.35267857142857145,"bottom":0.42791666666666667,"index":"5"},{"mode":"ellipse","left":0.855731974393531,"right":0.9352467991913747,"top":0.3816220238095238,"bottom":0.4282886904761905,"index":"6"},{"mode":"ellipse","left":0.39414167789757415,"right":0.5316079851752021,"top":0.3809375,"bottom":0.4571279761904762,"index":"7"},{"mode":"ellipse","left":0.03522995283018868,"right":0.1915641846361186,"top":0.6164136904761904,"bottom":0.7192708333333333,"index":"8"},{"mode":"ellipse","left":0.05675117924528302,"right":0.21308541105121293,"top":0.7045386904761904,"bottom":0.8016815476190476,"index":"9"}]})---");
  ASSERT_TRUE(ret);
}

TEST(json_read_unescaping) {
  {
	auto x=json::parse(R"({"data":"\ud55c\n\t\r"})");
	if (!x) {
	  fail("fail to parse");
	  return;
	}
	ASSERT_EQUAL(6,x["data"].size());
	ASSERT_EQUAL("한\n\t\r",x["data"]);
  }
  {
	// multiple r_string instance
	auto x=json::parse(R"({"data":"\ud55c\n\t\r"})");
	auto a=x["data"].get<string>();
	auto b=x["data"].get<string>();
	ASSERT_EQUAL(6,a.size());
	ASSERT_EQUAL(6,b.size());
	ASSERT_EQUAL(6,x["data"].size());
  }
}

TEST(json_write) {
  json x;
  x["message"]="hello world";
  ASSERT_EQUAL(R"({"message":"hello world"})",x.dump());
  x["message"]=std::string("string value");
  ASSERT_EQUAL(R"({"message":"string value"})",x.dump());
  x["message"]["x"]=3;
  ASSERT_EQUAL(R"({"message":{"x":3}})",x.dump());
  x["message"]["y"]=5;
  ASSERT_TRUE(R"({"message":{"x":3,"y":5}})"==x.dump()||R"({"message":{"y":5,"x":3}})"==x.dump());
  x["message"]=5.5;
  ASSERT_EQUAL(R"({"message":5.5})",x.dump());
  x["message"]=1234567890;
  ASSERT_EQUAL(R"({"message":1234567890})",x.dump());

  json y;
  y["scores"][0]=1;
  y["scores"][1]="king";
  y["scores"][2]=3.5;
  ASSERT_EQUAL(R"({"scores":[1,"king",3.5]})",y.dump());

  y["scores"][2][0]="real";
  y["scores"][2][1]=false;
  y["scores"][2][2]=true;
  ASSERT_EQUAL(R"({"scores":[1,"king",["real",false,true]]})",y.dump());

  y["scores"]["a"]["b"]["c"]=nullptr;
  ASSERT_EQUAL(R"({"scores":{"a":{"b":{"c":null}}}})",y.dump());

  y["scores"]=std::vector<int>{1,2,3};
  ASSERT_EQUAL(R"({"scores":[1,2,3]})",y.dump());
}

TEST(json_copy_r_to_w_to_r) {
  json r=json::parse(R"({"smallint":2,"bigint":2147483647,"fp":23.43,"fpsc":2.343e1,"str":"a string","trueval":true,"falseval":false,"nullval":null,"listval":[1,2,"foo","bar"],"obj":{"member":23,"other":"baz"}})");
  json w{r};
  json x=json::parse(w.dump()); // why no copy-ctor value -> rvalue?
  ASSERT_EQUAL(2,x["smallint"]);
  ASSERT_EQUAL(2147483647,x["bigint"]);
  ASSERT_EQUAL(23.43,x["fp"]);
  ASSERT_EQUAL(23.43,x["fpsc"]);
  ASSERT_EQUAL("a string",x["str"]);
  ASSERT_TRUE(true==x["trueval"].get<bool>());
  ASSERT_TRUE(false==x["falseval"].get<bool>());
  //ASSERT_TRUE(json::value_t::null==x["nullval"].get<nullptr>());
  ASSERT_EQUAL(4u,x["listval"].size());
  ASSERT_EQUAL(1,x["listval"][0]);
  ASSERT_EQUAL(2,x["listval"][1]);
  ASSERT_EQUAL("foo",x["listval"][2]);
  ASSERT_EQUAL("bar",x["listval"][3]);
  ASSERT_EQUAL(23,x["obj"]["member"]);
  ASSERT_EQUAL("member",x["obj"]["member"]);
  ASSERT_EQUAL("baz",x["obj"]["other"]);
  ASSERT_EQUAL("other",x["obj"]["other"]);
}

TEST(template_basic) {
  auto t=cc::mustache::template_t(R"---(attack of {{name}})---");
  json ctx;
  ctx["name"]="killer tomatoes";
  auto result=t.render(ctx);
  ASSERT_EQUAL("attack of killer tomatoes",result);
  //cc::mustache::load("basic.mustache");
}

TEST(template_load) {
  cc::mustache::directory(".");
  ofstream("test.mustache")<<R"---(attack of {{name}})---";
  auto t=cc::mustache::load("test.mustache");
  json ctx;
  ctx["name"]="killer tomatoes";
  auto result=t.render(ctx);
  ASSERT_EQUAL("attack of killer tomatoes",result);
  unlink("test.mustache");
}

int testmain() {
  bool failed=false;
  for (auto t:tests) {
	failed__=false;
	try {
	  //cerr << typeid(*t).name() << endl;
	  t->test();
	} catch (std::exception& e) {
	  fail(e.what());
	}
	if (failed__) {
	  cerr<<"F";
	  cerr<<'\t'<<typeid(*t).name()<<endl;
	  failed=true;
	} else
	  cerr<<".";
  }
  cerr<<endl;
  return failed?-1:0;
}

TEST(spell) {
  using namespace spell;
  static_assert(std::is_same<void,last_element_type<int,char,void>::type>::value,"last_element_type");
  static_assert(std::is_same<char,pop_back<int,char,void>::rebind<last_element_type>::type>::value,"pop_back");
  static_assert(std::is_same<int,pop_back<int,char,void>::rebind<pop_back>::rebind<last_element_type>::type>::value,"pop_back");
}

struct NullMiddleware {
  struct Ctx {};

  template <typename AllContext>
  void before_handle(Req&,Res&,Ctx&,AllContext&) {}

  template <typename AllContext>
  void after_handle(Req&,Res&,Ctx&,AllContext&) {}
};

struct NullSimpleMiddleware {
  struct Ctx {};

  void before_handle(Req& /*req*/,Res& /*res*/,Ctx& /*ctx*/) {}

  void after_handle(Req& /*req*/,Res& /*res*/,Ctx& /*ctx*/) {}
};


TEST(middleware_simple) {
  App<NullMiddleware,NullSimpleMiddleware> app;
  decltype(app)::server_t server(&app,LOCALHOST_ADDRESS,45451);
  ROUTE(app,"/")([&](cc::Req& req) {
	app.get_context<NullMiddleware>(req);
	app.get_context<NullSimpleMiddleware>(req);
	return "";
  });
}

struct IntSettingMiddleware {
  struct Ctx { int val; };

  template <typename AllContext>
  void before_handle(Req&,Res&,Ctx& ctx,AllContext&) {
	ctx.val=1;
  }

  template <typename AllContext>
  void after_handle(Req&,Res&,Ctx& ctx,AllContext&) {
	ctx.val=2;
  }
};

std::vector<std::string> test_middleware_context_vector;

struct FirstMW {
  struct Ctx {
	std::vector<string> v;
  };

  void before_handle(Req& /*req*/,Res& /*res*/,Ctx& ctx) {
	ctx.v.push_back("1 before");
  }

  void after_handle(Req& /*req*/,Res& /*res*/,Ctx& ctx) {
	ctx.v.push_back("1 after");
	test_middleware_context_vector=ctx.v;
  }
};

struct SecondMW {
  struct Ctx {};
  template <typename AllContext>
  void before_handle(Req& req,Res& res,Ctx&,AllContext& all_ctx) {
	all_ctx.template get<FirstMW>().v.push_back("2 before");
	if (req.url=="/break")
	  res.end();
  }

  template <typename AllContext>
  void after_handle(Req&,Res&,Ctx&,AllContext& all_ctx) {
	all_ctx.template get<FirstMW>().v.push_back("2 after");
  }
};

struct ThirdMW {
  struct Ctx {};
  template <typename AllContext>
  void before_handle(Req&,Res&,Ctx&,AllContext& all_ctx) {
	all_ctx.template get<FirstMW>().v.push_back("3 before");
  }

  template <typename AllContext>
  void after_handle(Req&,Res&,Ctx&,AllContext& all_ctx) {
	all_ctx.template get<FirstMW>().v.push_back("3 after");
  }
};

TEST(middleware_context) {

  static char buf[2048];
  // SecondMW depends on FirstMW (it uses all_ctx.get<FirstMW>)
  // so it leads to compile error if we remove FirstMW from definition
  // App<IntSettingMiddleware, SecondMW> app;
  // or change the order of FirstMW and SecondMW
  // App<IntSettingMiddleware, SecondMW, FirstMW> app;

  App<IntSettingMiddleware,FirstMW,SecondMW,ThirdMW> app;

  int x{};
  ROUTE(app,"/")([&](Req& req) {
	{
	  auto& ctx=app.get_context<IntSettingMiddleware>(req);
	  x=ctx.val;
	}
	{
	  auto& ctx=app.get_context<FirstMW>(req);
	  ctx.v.push_back("handle");
	}

	return "";
  });
  ROUTE(app,"/break")([&](Req& req) {
	{
	  auto& ctx=app.get_context<FirstMW>(req);
	  ctx.v.push_back("handle");
	}

	return "";
  });

  //decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
  //auto _ = async(launch::async, [&]{server.run();});
  auto _=async(launch::async,[&] {app.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  app.wait_for_server_start();
  std::string sendmsg="GET /\r\n\r\n";
  asio::io_service is;
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));


	c.send(asio::buffer(sendmsg));

	c.receive(asio::buffer(buf,2048));
	c.close();
  }
  {
	auto& out=test_middleware_context_vector;
	ASSERT_EQUAL(1,x);
	ASSERT_EQUAL(7,out.size());
	ASSERT_EQUAL("1 before",out[0]);
	ASSERT_EQUAL("2 before",out[1]);
	ASSERT_EQUAL("3 before",out[2]);
	ASSERT_EQUAL("handle",out[3]);
	ASSERT_EQUAL("3 after",out[4]);
	ASSERT_EQUAL("2 after",out[5]);
	ASSERT_EQUAL("1 after",out[6]);
  }
  std::string sendmsg2="GET /break\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));

	c.send(asio::buffer(sendmsg2));

	c.receive(asio::buffer(buf,2048));
	c.close();
  }
  {
	auto& out=test_middleware_context_vector;
	ASSERT_EQUAL(4,out.size());
	ASSERT_EQUAL("1 before",out[0]);
	ASSERT_EQUAL("2 before",out[1]);
	ASSERT_EQUAL("2 after",out[2]);
	ASSERT_EQUAL("1 after",out[3]);
  }
  app.stop();
}

TEST(middleware_cookieparser) {
  static char buf[2048];

  App<CookieParser> app;

  std::string value1;
  std::string value2;
  std::string value3;
  std::string value4;

  ROUTE(app,"/")([&](Req& req) {
	{
	  auto& ctx=app.get_context<CookieParser>(req);
	  value1=ctx.get_cookie("key1");
	  value2=ctx.get_cookie("key2");
	  value3=ctx.get_cookie("key3");
	  value4=ctx.get_cookie("key4");
	}

	return "";
  });

  //decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
  //auto _ = async(launch::async, [&]{server.run();});
  auto _=async(launch::async,[&] {app.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  app.wait_for_server_start();
  std::string sendmsg="GET /\r\nCookie: key1=value1; key2=\"val=ue2\"; key3=\"val\"ue3\"; key4=\"val\"ue4\"\r\n\r\n";
  asio::io_service is;
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));

	c.send(asio::buffer(sendmsg));

	c.receive(asio::buffer(buf,2048));
	c.close();
  }
  {
	ASSERT_EQUAL("value1",value1);
	ASSERT_EQUAL("val=ue2",value2);
	ASSERT_EQUAL("val\"ue3",value3);
	ASSERT_EQUAL("val\"ue4",value4);
  }
  app.stop();
}

TEST(bug_quick_repeated_request) {
  static char buf[2048];

  App<> app;

  ROUTE(app,"/")([&] {
	return "hello";
  });

  //decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
  //auto _ = async(launch::async, [&]{server.run();});
  auto _=async(launch::async,[&] {app.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  app.wait_for_server_start();
  std::string sendmsg="GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  asio::io_service is;
  {
	std::vector<std::future<void>> v;
	for (int i=0; i<5; i++) {
	  v.push_back(async(launch::async,
						[&] {
		asio::ip::tcp::socket c(is);
		c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));

		for (int j=0; j<5; j++) {
		  c.send(asio::buffer(sendmsg));

		  size_t received=c.receive(asio::buffer(buf,2048));
		  ASSERT_EQUAL("hello",std::string(buf+received-5,buf+received));
		}
		c.close();
	  }));
	}
  }
  app.stop();
}

TEST(simple_url_params) {
  static char buf[2048];

  App<> app;

  query_string last_url_params;

  ROUTE(app,"/params")
	([&last_url_params](cc::Req& req) {
	last_url_params=std::move(req.url_params);
	return "OK";
  });

  ///params?h=1&foo=bar&lol&count[]=1&count[]=4&pew=5.2

  //decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
  //auto _ = async(launch::async, [&]{server.run();});
  auto _=async(launch::async,[&] {app.bindaddr(LOCALHOST_ADDRESS).set_port(45451).run();});
  app.wait_for_server_start();
  asio::io_service is;
  std::string sendmsg;

  // check empty params
  sendmsg="GET /params\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	stringstream ss;
	ss<<last_url_params;

	ASSERT_EQUAL("[  ]",ss.str());
  }
  // check single presence
  sendmsg="GET /params?foobar\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_TRUE(last_url_params.get("missing")==nullptr);
	ASSERT_TRUE(last_url_params.get("foobar")!=nullptr);
	ASSERT_TRUE(last_url_params.get_list("missing").empty());
  }
  // check multiple presence
  sendmsg="GET /params?foo&bar&baz\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_TRUE(last_url_params.get("missing")==nullptr);
	ASSERT_TRUE(last_url_params.get("foo")!=nullptr);
	ASSERT_TRUE(last_url_params.get("bar")!=nullptr);
	ASSERT_TRUE(last_url_params.get("baz")!=nullptr);
  }
  // check single value
  sendmsg="GET /params?hello=world\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_EQUAL(string(last_url_params.get("hello")),"world");
  }
  // check multiple value
  sendmsg="GET /params?hello=world&left=right&up=down\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_EQUAL(string(last_url_params.get("hello")),"world");
	ASSERT_EQUAL(string(last_url_params.get("left")),"right");
	ASSERT_EQUAL(string(last_url_params.get("up")),"down");
  }
  // check multiple value, multiple types
  sendmsg="GET /params?int=100&double=123.45&boolean=1\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);
	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_EQUAL(boost::lexical_cast<int>(last_url_params.get("int")),100);
	ASSERT_EQUAL(boost::lexical_cast<double>(last_url_params.get("double")),123.45);
	ASSERT_EQUAL(boost::lexical_cast<bool>(last_url_params.get("boolean")),true);
  }
  // check single array value
  sendmsg="GET /params?tmnt[]=leonardo\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);

	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_TRUE(last_url_params.get("tmnt")==nullptr);
	ASSERT_EQUAL(last_url_params.get_list("tmnt").size(),1);
	ASSERT_EQUAL(string(last_url_params.get_list("tmnt")[0]),"leonardo");
  }
  // check multiple array value
  sendmsg="GET /params?tmnt[]=leonardo&tmnt[]=donatello&tmnt[]=raphael\r\n\r\n";
  {
	asio::ip::tcp::socket c(is);

	c.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string(LOCALHOST_ADDRESS),45451));
	c.send(asio::buffer(sendmsg));
	c.receive(asio::buffer(buf,2048));
	c.close();

	ASSERT_EQUAL(last_url_params.get_list("tmnt").size(),3);
	ASSERT_EQUAL(string(last_url_params.get_list("tmnt")[0]),"leonardo");
	ASSERT_EQUAL(string(last_url_params.get_list("tmnt")[1]),"donatello");
	ASSERT_EQUAL(string(last_url_params.get_list("tmnt")[2]),"raphael");
  }
  app.stop();
}

TEST(get) {
  App<> app;
  int x=1;
  app.get("/")
	([&] {
	x=2;
	return "";
  });

  app.get("/set4")
	([&](Req&) {
	x=4;
	return "";
  });
  app.get("/set5")
	([&](Req&,Res& res) {
	x=5;
	res.end();
  });

  app.get("/set_int/<int>")
	([&](int y) {
	x=y;
	return "";
  });

  try {
	app.get("/invalid_test/<double>/<path>")
	  ([]() {
	  return "";
	});
	fail();
  } catch (std::exception&) {
  }

  // app is in an invalid state when route throws an exception.
  try {
	app.validate();
	fail();
  } catch (std::exception&) {
  }

  {
	Req req;
	Res res;
	req.url="/";
	app.handle(req,res);
	ASSERT_EQUAL(x,2);
  }
  {
	Req req;
	Res res;
	req.url="/set_int/42";
	app.handle(req,res);
	ASSERT_EQUAL(x,42);
  }
  {
	Req req;
	Res res;
	req.url="/set5";
	app.handle(req,res);
	ASSERT_EQUAL(x,5);
  }
  {
	Req req;
	Res res;
	req.url="/set4";
	app.handle(req,res);
	ASSERT_EQUAL(x,4);
  }
}

int main() {
  return testmain();
}