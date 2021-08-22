#pragma once
//#define CROW_DISABLE_HOME
//#define CROW_ENABLE_SSL
#define CROW_FILE_TIME "max-age=1500,immutable"//Static resource cache seconds(= 25 minute)
#define CROW_STATIC_DIRECTORY "static/"//Make sure you bring '/' with you at the end
#define CROW_HOME_PAGE "index.html"//default home page(app.server_name(?))
#define CROW_SERVER_NAME "Crow/1.0"//default server name(app.home(?))
//Cors config
#define ACAO 1
//#define AccessControlAllowOrigin "*"
#define ACAC 4
//#define AccessControlAllowCredentials "true"
#define ACAH 57
//#define AccessControlAllowHeaders "content-type,cache-control,x-requested-with,authorization"
#define ACAM 32
//#define AccessControlAllowMethods "GET,POST,DELETE,PUT,OPTIONS,HEAD"

namespace crow {
  //SqlDataBase will automatically shut down after 8 hours (28800 seconds) of
  // inactivity by default (determined by the mechanism provided by the server)
  //typedef sql_database<sqlType,time_wait> D;time_wait default 28800
  typedef sql_database<mysql, 99> Mysql;
  typedef sql_database<pgsql, 99> Pgsql;
//-------------- utf8 / GB2312 / GBK --------------
#define D_mysql() crow::Mysql("127.0.0.1","test","root","",3306,SYS_IS_UTF8?"utf8":"GBK")
#define D_pgsql() crow::Pgsql("127.0.0.1","test","Asciphx","",5432,SYS_IS_UTF8?"utf8":"GBK")
//------ Use GBK or GB2312 to support Chinese ------
//---- SQLite can only support default encoding ----
#define D_sqlite(path) crow::Sqlite(path)
#define D_() crow::Mysql("127.0.0.1","test","root","",3306,SYS_IS_UTF8?"utf8":"GBK")
//example to use: auto d = D_();
}
