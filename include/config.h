#pragma once
//#define CROW_DISABLE_HOME
//#define CROW_ENABLE_SSL
#define CROW_FILE_TIME "max-age=1500,immutable"//Static resource cache seconds(= 25 minute)
#define CROW_STATIC_DIRECTORY "static/"//Make sure you bring '/' with you at the end
#define CROW_SERVER_NAME "Crow/1.0"//the server name config
#define CROW_HOME_PAGE "index.html"//default home page(app.home(?))
//Cors config
#define ACAO 1
//#define AccessControlAllowOrigin "*"
#define ACAC 4
//#define AccessControlAllowCredentials "true"
#define ACAH 57
//#define AccessControlAllowHeaders "content-type,cache-control,x-requested-with,authorization"
#define ACAM 32
//#define AccessControlAllowMethods "GET,POST,DELETE,PUT,OPTIONS,HEAD"
#define SHOW_SERVER_NAME false //It is better to set false for 1-core 2G server
