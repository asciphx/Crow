#pragma once
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>

#include "cc/settings.h"
namespace cc {
  enum class LogLevel {
	DEBUG=0,INFO,WARNING,ERR,CRITICAL,
  };
  class ILogHandler {
	public:virtual void log(std::string message,LogLevel level)=0;
  };
  class CerrLogHandler : public ILogHandler {
	public:void log(std::string message,LogLevel) override { std::cerr<<message; }
  };

  class logger {
	private:
	static std::string timestamp() {
	  char date[32];
	  time_t t=time(0);
	  tm my_tm;
#if defined(_MSC_VER) || defined(__MINGW32__)
	  localtime_s(&my_tm,&t);
#else
	  localtime_r(&t,&my_tm);
#endif
	  size_t sz=strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",&my_tm);
	  return std::string(date,date+sz);
	}
	public:
	logger(std::string prefix,LogLevel level): level_(level) {
#ifdef DEFAULT_ENABLE_LOGGING
	  stringstream_<<"("<<timestamp()<<") ["<<prefix<<"] ";
#endif
	}
	~logger() {
#ifdef DEFAULT_ENABLE_LOGGING
	  if (level_>=get_current_log_level()) {
		stringstream_<<std::endl;
		get_handler_ref()->log(stringstream_.str(),level_);
	  }
#endif
	}
	template <typename T>
	logger& operator<<(T const &value) {

#ifdef DEFAULT_ENABLE_LOGGING
	  if (level_>=get_current_log_level()) {
		stringstream_<<value;
	  }
#endif
	  return *this;
	}
	static void setLogLevel(LogLevel level) {
	  get_log_level_ref()=level;
	}
	static void setHandler(ILogHandler* handler) {
	  get_handler_ref()=handler;
	}
	static LogLevel get_current_log_level() {
	  return get_log_level_ref();
	}
	private:
	static LogLevel& get_log_level_ref() {
	  static LogLevel current_level=(LogLevel)DEFAULT_LOG_LEVEL;
	  return current_level;
	}
	static ILogHandler*& get_handler_ref() {
	  static CerrLogHandler default_handler;
	  static ILogHandler* current_handler=&default_handler;
	  return current_handler;
	}
	std::ostringstream stringstream_;
	LogLevel level_;
  };
}
#define LOG_CRITICAL   \
        if (cc::logger::get_current_log_level() <= cc::LogLevel::CRITICAL) \
            cc::logger("CRITICAL", cc::LogLevel::CRITICAL)
#define LOG_ERROR      \
        if (cc::logger::get_current_log_level() <= cc::LogLevel::ERR) \
            cc::logger("ERROR   ", cc::LogLevel::ERR)
#define LOG_WARNING    \
        if (cc::logger::get_current_log_level() <= cc::LogLevel::WARNING) \
            cc::logger("WARNING ", cc::LogLevel::WARNING)
#define LOG_INFO       \
        if (cc::logger::get_current_log_level() <= cc::LogLevel::INFO) \
            cc::logger("INFO    ", cc::LogLevel::INFO)
#define LOG_DEBUG      \
        if (cc::logger::get_current_log_level() <= cc::LogLevel::DEBUG) \
            cc::logger("DEBUG   ", cc::LogLevel::DEBUG)
