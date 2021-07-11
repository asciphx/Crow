#pragma once
#include <boost/asio.hpp>
#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include "crow/settings.h"
#if BOOST_VERSION > 106900
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif
namespace crow {
  using namespace boost;//unsigned int utimeout_milli = 1500;int nSendBuf = 20*1024,nRecvBuf = 20*1024;
  using tcp=asio::ip::tcp;
  ///A wrapper for the asio::ip::tcp::socket and asio::ssl::stream
  struct SocketAdaptor {
    using Ctx=void;
    SocketAdaptor(asio::io_service& io_service,Ctx*): socket_(io_service) {}

    asio::io_service& get_io_service() {
      return GET_IO_SERVICE(socket_);
      //setsockopt(socket_.native_handle(),SOL_SOCKET,SO_SNDBUF,(const char*)&crow::nSendBuf,sizeof(int));
      //setsockopt(socket_.native_handle(),SOL_SOCKET,SO_RCVBUF,(const char*)&crow::nRecvBuf,sizeof(int));
//#if defined __linux__ || defined __APPLE__// platform-specific switch
//	  struct timeval tv;// assume everything else is posix
//	  tv.tv_sec = timeout_milli/1000;tv.tv_usec = (timeout_milli%1000)*1000;
//	  setsockopt(socket_.native_handle(),SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));//Receiving time limit
//	  setsockopt(socket_.native_handle(),SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));//Sending time limit
//#else
//	  int32_t timeout = crow::utimeout_milli; // use windows-specific time
//	  setsockopt(socket_.native_handle(),SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
//	  setsockopt(socket_.native_handle(),SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
//#endif
    }

    /// Get the TCP socket handling data trasfers, regardless of what layer is handling transfers on top of the socket.
    tcp::socket& raw_socket() {
      return socket_;
    }

    /// Get the object handling data transfers, this can be either a TCP socket or an SSL stream (if SSL is enabled).
    tcp::socket& socket() {
      return socket_;
    }

    tcp::endpoint remote_endpoint() {
      return socket_.remote_endpoint();
    }

    bool is_open() {
      return socket_.is_open();
    }
    void cancel() {
      socket_.cancel();
    }
    void close() {
      system::error_code ec;
      socket_.close(ec);
    }

    void shutdown_readwrite() {
      system::error_code ec;
      socket_.shutdown(asio::socket_base::shutdown_type::shutdown_both,ec);
    }

    void shutdown_write() {
      system::error_code ec;
      socket_.shutdown(asio::socket_base::shutdown_type::shutdown_send,ec);
    }

    void shutdown_read() {
      system::error_code ec;
      socket_.shutdown(asio::socket_base::shutdown_type::shutdown_receive,ec);
    }

    template <typename F>
    void start(F f) {
      f(system::error_code());
    }

    tcp::socket socket_;
  };

#ifdef CROW_ENABLE_SSL
  struct SSLAdaptor {
    using Ctx=asio::ssl::context;
    using ssl_socket_t=asio::ssl::stream<tcp::socket>;
    SSLAdaptor(asio::io_service& io_service,Ctx* ctx)
      : ssl_socket_(new ssl_socket_t(io_service,*ctx)) {}

    asio::ssl::stream<tcp::socket>& socket() {
      return *ssl_socket_;
    }

    tcp::socket::lowest_layer_type&
      raw_socket() {
      return ssl_socket_->lowest_layer();
    }

    tcp::endpoint remote_endpoint() {
      return raw_socket().remote_endpoint();
    }

    bool is_open() {
      return ssl_socket_?raw_socket().is_open():false;
    }

    void close() {
      if (is_open()) {
        system::error_code ec;
        raw_socket().close(ec);
      }
    }

    void shutdown_readwrite() {
      if (is_open()) {
        system::error_code ec;
        raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_both,ec);
      }
    }

    void shutdown_write() {
      if (is_open()) {
        system::error_code ec;
        raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_send,ec);
      }
    }

    void shutdown_read() {
      if (is_open()) {
        system::error_code ec;
        raw_socket().shutdown(asio::socket_base::shutdown_type::shutdown_receive,ec);
      }
    }

    asio::io_service& get_io_service() {
      return GET_IO_SERVICE(raw_socket());
    }

    template <typename F>
    void start(F f) {
      ssl_socket_->async_handshake(asio::ssl::stream_base::server,
                                   [f](const system::error_code& ec) {
        f(ec);
      });
    }
    std::unique_ptr<asio::ssl::stream<tcp::socket>> ssl_socket_;
  };
#endif
}
