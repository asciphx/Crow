#pragma once// settings for crow
#define CROW_ENABLE_LOGGING
/* #ifdef - enables ssl */
//#define CROW_ENABLE_SSL
//#define CROW_DISABLE_HOME//The route of '/' can be customized only after it is disabled.
#define CROW_LOG_LEVEL 1

#define CROW_FILE_TIME "max-age=900,immutable"//Static resource cache seconds(= 15 minute)
#define CROW_STATIC_DIRECTORY "static/"//Make sure you bring '/' with you at the end
#define CROW_HOME_PAGE "index.html"
#define CROW_SERVER_NAME "Crow/0.7-beta"
#define CROW_DEFAULT_PORT 80

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define CROW_MSVC_WORKAROUND
#define constexpr const
#define noexcept throw()
#endif
#endif
