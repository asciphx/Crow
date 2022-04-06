#pragma once
//#define DISABLE_HOME
//#define ENABLE_SSL
#define FILE_TIME "max-age=30,immutable"//Static resource cache seconds(= 25 minute)
#define STATIC_DIRECTORY "static/"//Make sure you bring '/' with you at the end
#define UPLOAD_DIRECTORY "upload/"//Make sure you bring '/' with you at the end
#define SERVER_NAME "Crow/1.1"//the server name config
#define HOME_PAGE "index.html"//default home page(app.home(?))
//Cors config
#define AccessControlAllowOrigin "*"
//#define AccessControlAllowCredentials "true"
//#define AccessControlAllowHeaders "content-type,cache-control,x-requested-with,authorization"
#define AccessControlAllowMethods "GET,POST,DELETE,PUT,OPTIONS,HEAD"
#define SHOW_SERVER_NAME 1
