
#pragma once
#include <boost/asio.hpp>
#ifdef ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include "cc/settings.h"
#if BOOST_VERSION > 106900
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif
namespace cc { using namespace boost; static unsigned int utimeout_milli = 3500; using tcp=asio::ip::tcp; 
#if defined __linux__ || defined __APPLE__
 struct timeval tv { utimeout_milli / 1000, utimeout_milli };
#endif
 struct SocketAdaptor { using Ctx=void; SocketAdaptor(asio::io_service& io_service,Ctx*): socket_(io_service) {  
#if defined __linux__ || defined __APPLE__
 setsockopt(socket_.native_handle(),SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)); setsockopt(socket_.native_handle(),SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
#else
 setsockopt(socket_.native_handle(),SOL_SOCKET,SO_RCVTIMEO,(const char*)&cc::utimeout_milli,sizeof(cc::utimeout_milli)); setsockopt(socket_.native_handle(),SOL_SOCKET,SO_SNDTIMEO,(const char*)&cc::utimeout_milli,sizeof(cc::utimeout_milli));
#endif
 } asio::io_service& get_io_service() { return GET_IO_SERVICE(socket_); }  tcp::socket& raw_socket() { return socket_; }  tcp::socket& socket() { return socket_; } tcp::endpoint remote_endpoint() { return socket_.remote_endpoint(); } bool is_open() { return socket_.is_open(); } void cancel() { socket_.cancel(); } void close() { system::error_code ec; socket_.close(ec); } void shutdown_readwrite() { system::error_code ec; socket_.shutdown(asio::socket_base::shutdown_type::shutdown_both,ec); } void shutdown_write() { system::error_code ec; socket_.shutdown(asio::socket_base::shutdown_type::shutdown_send,ec); } void shutdown_read() { system::error_code ec; socket_.shutdown(asio::socket_base::shutdown_type::shutdown_receive,ec); } template <typename F> void start(F f) { f(system::error_code()); } tcp::socket socket_; };
#ifdef ENABLE_SSL
 struct SSLAdaptor { using Ctx=asio::ssl::context; using ssl_socket_t=asio::ssl::stream<tcp::socket>; SSLAdaptor(asio::io_service& io_service,Ctx* ctx) : ssl_socket_(new ssl_socket_t(io_service,*ctx)) {} asio::ssl::stream<tcp::socket>& socket() { return *ssl_socket_; } tcp::socket::lowest_layer_type& raw_socket() { return ssl_socket_->lowest_layer(); } tcp::endpoint remote_endpoint() { return raw_socket().remote_endpoint(); } bool is_open() { return ssl_socket_?raw_socket().is_open():false; } void close() { if (is_open()) { system::error_code ec; raw_socket().close(ec); } } void shutdown_readwrite() { if (is_open()) { system::error_code ec; raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_both,ec); } } void shutdown_write() { if (is_open()) { system::error_code ec; raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_send,ec); } } void shutdown_read() { if (is_open()) { system::error_code ec; raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_receive,ec); } } asio::io_service& get_io_service() { return GET_IO_SERVICE(raw_socket()); } template <typename F> void start(F f) { ssl_socket_->async_handshake(asio::ssl::stream_base::server,  [f](const system::error_code& ec) { f(ec); }); } std::unique_ptr<asio::ssl::stream<tcp::socket>> ssl_socket_; };
#endif
}