
#pragma once
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include "config.h"
namespace cc { enum class LogLevel { DEBUG = 1, INFO, WARNING, ERR, CRITICAL, }; class ILogHandler { public:virtual void log(std::string message, LogLevel level) = 0; }; class CerrLogHandler : public ILogHandler { public:void log(std::string message, LogLevel) override { std::cerr << message; } }; class logger { private: static std::string timestamp() { char date[32]; time_t t = time(0); tm my_tm;
#if defined(_MSC_VER) || defined(__MINGW32__)
 localtime_s(&my_tm, &t);
#else
 localtime_r(&t, &my_tm);
#endif
 size_t sz = strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &my_tm); return std::string(date, date + sz); } public: logger(std::string prefix, LogLevel level) : level_(level) {
#if DEFAULT_ENABLE_LOGGING
 stringstream_ << "(" << timestamp() << ") [" << prefix << "] ";
#endif
 } ~logger() {
#if DEFAULT_ENABLE_LOGGING
 if (level_ >= get_current_log_level()) { stringstream_ << std::endl; get_handler_ref()->log(stringstream_.str(), level_); }
#endif
 } template <typename T> logger& operator<<(T const& value) {
#if DEFAULT_ENABLE_LOGGING
 if (level_ >= get_current_log_level()) { stringstream_ << value; }
#endif
 return *this; } static void setLogLevel(LogLevel level) { get_log_level_ref() = level; } static void setHandler(ILogHandler* handler) { get_handler_ref() = handler; } static LogLevel get_current_log_level() { return get_log_level_ref(); } private: static LogLevel& get_log_level_ref() { static LogLevel current_level = (LogLevel)DEFAULT_LOG_LEVEL; return current_level; } static ILogHandler*& get_handler_ref() { static CerrLogHandler default_handler; static ILogHandler* current_handler = &default_handler; return current_handler; } std::ostringstream stringstream_; LogLevel level_; };}
#if DEFAULT_ENABLE_LOGGING
#define LOG_CRITICAL(mes)   if (cc::logger::get_current_log_level() == cc::LogLevel::CRITICAL)  cc::logger("CRITICAL", cc::LogLevel::CRITICAL) << mes;

#define LOG_ERROR(mes)  if (cc::logger::get_current_log_level() >= cc::LogLevel::ERR)  cc::logger("ERROR  ", cc::LogLevel::ERR) << mes;

#define LOG_WARNING(mes)  if (cc::logger::get_current_log_level() == cc::LogLevel::WARNING)  cc::logger("WARNING ", cc::LogLevel::WARNING) << mes;

#define LOG_INFO(mes)   if (cc::logger::get_current_log_level() <= cc::LogLevel::INFO)  cc::logger("INFO ", cc::LogLevel::INFO) << mes;

#define LOG_DEBUG(mes)  if (cc::logger::get_current_log_level() == cc::LogLevel::DEBUG)  cc::logger("DEBUG  ", cc::LogLevel::DEBUG) << mes;

#else
#define LOG_CRITICAL(mes)
#define LOG_ERROR(mes)
#define LOG_WARNING(mes)
#define LOG_INFO(mes)
#define LOG_DEBUG(mes)
#endif