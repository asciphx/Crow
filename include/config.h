#pragma once
//#define CROW_DISABLE_HOME
//#define CROW_ENABLE_SSL
#define CROW_FILE_TIME "max-age=900,immutable"//Static resource cache seconds(= 15 minute)
#define CROW_STATIC_DIRECTORY "static/"//Make sure you bring '/' with you at the end
#define CROW_HOME_PAGE "index.html"
#define CROW_SERVER_NAME "Crow/0.9"
#define ACAO 1
#define AccessControlAllowOrigin "*"
//The following will seriously affect the performance, so I have no comment
#define ACAC 4
//#define AccessControlAllowCredentials "true"
#define ACAH 57
//#define AccessControlAllowHeaders "content-type,cache-control,x-requested-with,authorization"
#define ACAM 32
//#define AccessControlAllowMethods "GET,POST,DELETE,PUT,OPTIONS,HEAD"