﻿﻿![乌鸦标志](http://i.imgur.com/wqivvjK.jpg)

乌鸦是Web的C++微服务框架，支持mac,linux,windows,三大平台，开发速度最快最迅速最猛烈，下一步即将支持ORM。目前估测能在techempower应该可以排到世界前五。
### [示例(example_vs)](http://8.129.58.72:8080/)🚀
（灵感来自 Python Flask）[由Asciphx提供的分支]

[![Travis Build](https://travis-ci.org/ipkn/crow.svg?branch=master)](https://travis-ci.org/ipkn/crow)
[![Coverage Status](https://coveralls.io/repos/github/asciphx/Crow/badge.svg?branch=master)](https://coveralls.io/github/asciphx/Crow?branch=master)

```c++
#include "cc.h"
int main(){
    cc::SimpleApp app;
    app["/"]([](){
        return u8"你好 世界！";
    });
    app.port(18080).multithreaded().run();
}
```

## 特点
- 简易路由，类似于Python Flask
- 类型安全处理程序（参见示例），非常快
 ![基准结果](./Benchmark.png)
- 更多关于[crow benchmark]的数据(https://github.com/ipkn/crow-benchmark)
- 第三方JSON解析器[Nlohmann json](https://github.com/nlohmann/json)用于静态反射，输出json。
- [Mustache](http://mustache.github.io/)基于模板库（crow:：mustache）
- 中间件支持，Websocket支持
- 支持静态资源,并且默认在'static/'目录
- 模块化开发，效率非常高，代码极简
## 仍在开发中
-~~内置ORM~~
-检查[sqlpp11](https://github.com/rbock/sqlpp11)如果你想要的话。
- 现在允许在'config.h'专门配置CORS
## 示例
#### 上传文件
```c++
  app.post("/upload")([](cc::Req& req) {
	  cc::Parser<2048> msg(req);
	  json j = json::object();
	  for (auto p : msg.params) {
	    if (!p.size) j[p.key] = p.value; else j[p.key] = p.filename;
	  }
	  return j;
	});
```
#### sql查询
```c++
  app["/sql"]([] {
	auto q = d.conn();
	//std::tuple<int, std::string> ds=q("select id,name from users_test where id = 1").template r__<int,std::string>();
	//std::cout<<std::get<0>(ds)<<std::get<1>(ds);
	int i = 0; q("SELECT 200+2").r__(i);
	std::string s; q(u8"SELECT '你好 世界！'").r__(s);
	return Res(i,s);
  });
```
#### 静态反射
```c++
  app["/list"]([]() {
	User u; List list{ &u }; json::parse(list, R"({"user":{"is":false,"age":25,"weight":50.6,"name":"deaod"},
	  "userList":[{"is":true,"weight":52.0,"age":23,"state":true,"name":"wwzzgg"},
	  {"is":true,"weight":51.0,"name":"best","age":26}]})");
	json json_output = json(list);
	return json_output;
  });
```
#### 服务端渲染
```c++
  app.default_route()([] {
	char name[64];gethostname(name,64);
	json j{{"servername",name}};
	return mustache::load("404NotFound.html").render(j);
  });
```

#### JSON响应
```c++
app["/json"]([]{
    cc::json x;
	x["message"] = u8"你好 世界！";
	x["double"]=3.1415926;
	x["int"]=2352352;
	x["true"]=true;
	x["false"]=false;
	x["null"]=nullptr;
	x["bignumber"]=2353464586543265455;
    return x;
});
```

#### 论据
```c++
app["/hello/<int>"]([](int count){
    if (count > 100) return cc::Res(400);
    std::ostringstream os;
    os << count << " bottles of beer!";
    return cc::Res(os.str());
});
```
编译时的处理程序参数类型检查 
```c++
// 编译错误，消息"处理程序类型与URL参数不匹配"
app["/another/<int>"]([](int a, int b){
    return cc::Res(500);
});
```

#### 处理JSON请求
```c++
app.post("/add_json")
([](cc::Req& req){
    auto x = cc::json::load(req.body);
    if (!x) return cc::Res(400);
	int sum=x["a"].get<int>()+x["b"].get<int>();
    std::ostringstream os;
    os << sum;
    return cc::Res{os.str()};
});
```

## 如何构建
如果您只想使用crow，请复制amalgamate/all.h 并包含它。

### 要求
- C++ 编译器，支持C++17（用G++测试>8.0）
- 任何版本的boost库
- 构建示例的CMake
- 建议与tcmalloc/jemalloc链接以提高速度。
- 现在支持VS2019，功能有限（只有url的运行时检查可用。）

### 建筑（测试、示例）
建议使用CMake进行源代码外构建。
```
mkdir build
cd build
cmake ..
make
```

可以使用以下命令运行测试：

```
ctest
```

### 安装缺少的依赖项
#### Ubuntu
    sudo apt-get install build-essential libtcmalloc-minimal4 && sudo ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so
#### OSX
    brew install boost google-perftools

#### windows

>首次安装boost
>第二次修改CmakeLists.txt

##### 讨论群号[1082037157]

### 归属
Crow使用以下库。  
http解析器 https://github.com/nodejs/llhttp

版权所有Fedor Indutny，2018。
特此免费准许任何获得本软件和相关文档文件的副本
“软件”），不受限制地处理软件，包括
但不限于使用、复制、修改、合并、发布，
分发、再许可和/或销售软件的副本，并允许
向其提供软件的人员，根据以下情况：
包括上述版权声明和本许可声明在软件的所有副本或主要部分。
本软件按“原样”提供，不提供任何形式的保证
或暗示，包括但不限于适销性、特定用途的适用性和非侵权性。在
任何情况下，作者或版权持有人均不对任何索赔负责，
损害赔偿或其他责任，无论是在合同诉讼、侵权诉讼或
否则，由软件或在软件中使用或进行其他交易。

qs_parse https://github.com/bartgrantham/qs_parse  

版权所有（c）2010 Bart Grantham
在这里，任何获得副本的人都可以免费获得许可
本软件及相关文档文件（“软件”），以处理
在软件中不受限制，包括但不限于权利
使用、复制、修改、合并、发布、分发、再许可和/或销售
软件的副本，并允许使用软件的人员
按照以下条件提供：
上述版权声明和本许可声明应包含在
软件的所有副本或主要部分。

TinySHA1 https://github.com/mohaps/TinySHA1

TinySHA1-SHA1算法的一个只包含报头的实现。基于boost::uuid::details中的实现
Cmohaps@gmail.com
特此授予出于任何目的使用、复制、修改和分发本软件的许可，无论是否收费，前提是上述版权声明和本许可声明出现在所有副本中。
本软件按“原样”提供，作者不承担与本软件有关的所有保证，包括对适销性和适用性的所有暗示保证。

json https://github.com/nlohmann/json

版权所有（c）2013-2021 Niels Lohmann
在这里，任何获得副本的人都可以免费获得许可
本软件及相关文档文件（“软件”），以处理
在软件中不受限制，包括但不限于权利
使用、复制、修改、合并、发布、分发、再许可和/或销售
软件的副本，并允许使用软件的人员
按照以下条件提供：
上述版权声明和本许可声明应包含在所有
软件的副本或大部分。
本软件按“原样”提供，不提供任何形式的明示或明示担保
默示，包括但不限于适销性保证，
适用于特定目的和非侵犯性。在任何情况下
作者或版权持有人对任何索赔、损害或其他
无论是在合同诉讼、侵权诉讼或其他诉讼中，
不属于或与本软件有关，或与本软件的使用或其他交易有关
软件。