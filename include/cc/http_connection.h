#pragma once
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <atomic>
#include <chrono>
#include <vector>

#include "cc/llhttp.h"
#include "cc/http_response.h"
#include "cc/logging.h"
#include "cc/settings.h"
#include "cc/detail.h"
#include "cc/middleware_context.h"
#include "cc/socket_adaptors.h"
#include "cc/compression.h"
static char Res_server_tag[9] = "Server: ", Res_content_length_tag[17] = "Content-Length: ", Res_http_status[10] = "HTTP/1.1 ",
RES_AcC[35] = "Access-Control-Allow-Credentials: ", RES_t[5] = "true", RES_AcM[] = "Access-Control-Allow-Methods: ",
RES_AcH[31] = "Access-Control-Allow-Headers: ", RES_AcO[30] = "Access-Control-Allow-Origin: ", Res_expect[26] = "HTTP/1.1 100 Continue\r\n\r\n",
Res_date_tag[7] = "Date: ", Res_content_length[15] = "content-length", Res_seperator[3] = ": ", Res_crlf[3] = "\r\n", Res_loc[9] = "location";
namespace cc {
  namespace detail {
	template <typename MW>struct check_before_handle_arity_3_const {
	  template <typename T, void(T::*)(Req&, Res&, typename MW::Ctx&)const = &T::before_handle >
	  struct get {};
	};
	template <typename MW>struct check_before_handle_arity_3 {
	  template <typename T, void(T::*)(Req&, Res&, typename MW::Ctx&) = &T::before_handle >
	  struct get {};
	};
	template <typename MW>struct check_after_handle_arity_3_const {
	  template <typename T, void(T::*)(Req&, Res&, typename MW::Ctx&)const = &T::after_handle >
	  struct get {};
	};
	template <typename MW>struct check_after_handle_arity_3 {
	  template <typename T, void(T::*)(Req&, Res&, typename MW::Ctx&) = &T::after_handle >
	  struct get {};
	};
	template <typename T>struct is_before_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3_const<T>::template get<C>*);
	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3<T>::template get<C>*);
	  template <typename C>
	  static std::false_type f(...);
	public: static const bool value = decltype(f<T>(nullptr))::value;
	};
	template <typename T>
	struct is_after_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3_const<T>::template get<C>*);
	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3<T>::template get<C>*);
	  template <typename C>
	  static std::false_type f(...);
	public: static const bool value = decltype(f<T>(nullptr))::value;
	};
	template <typename MW, typename Context, typename ParentContext>
	typename std::enable_if<!is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw, Req& req, Res& res, Context& ctx, ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req, res, ctx.template get<MW>(), ctx);
	}
	template <typename MW, typename Context, typename ParentContext>
	typename std::enable_if<is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw, Req& req, Res& res, Context& ctx, ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req, res, ctx.template get<MW>());
	}
	template <typename MW, typename Context, typename ParentContext>
	typename std::enable_if<!is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw, Req& req, Res& res, Context& ctx, ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req, res, ctx.template get<MW>(), ctx);
	}
	template <typename MW, typename Context, typename ParentContext>
	typename std::enable_if<is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw, Req& req, Res& res, Context& ctx, ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req, res, ctx.template get<MW>());
	}
	template <int N, typename Context, typename Container, typename CurrentMW, typename ... Middlewares>
	bool middleware_call_helper(Container& middlewares, Req& req, Res& res, Context& ctx) {
	  using parent_context_t = typename Context::template partial<N - 1>;
	  before_handler_call<CurrentMW, Context, parent_context_t>(std::get<N>(middlewares), req, res, ctx, static_cast<parent_context_t&>(ctx));
	  if (res.is_completed()) {
		after_handler_call<CurrentMW, Context, parent_context_t>(std::get<N>(middlewares), req, res, ctx, static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  if (middleware_call_helper<N + 1, Context, Container, Middlewares...>(middlewares, req, res, ctx)) {
		after_handler_call<CurrentMW, Context, parent_context_t>(std::get<N>(middlewares), req, res, ctx, static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  return false;
	}
	template <int N, typename Context, typename Container>
	bool middleware_call_helper(Container& /*middlewares*/, Req& /*req*/, Res& /*res*/, Context& /*ctx*/) {
	  return false;
	}
	template <int N, typename Context, typename Container>
	typename std::enable_if<(N < 0)>::type
	  after_handlers_call_helper(Container& /*middlewares*/, Context& /*context*/, Req& /*req*/, Res& /*res*/) {}
	template <int N, typename Context, typename Container>
	typename std::enable_if<(N == 0)>::type after_handlers_call_helper(Container& middlewares, Context& ctx, Req& req, Res& res) {
	  using parent_context_t = typename Context::template partial<N - 1>;
	  using CurrentMW = typename std::tuple_element<N, typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW, Context, parent_context_t>(std::get<N>(middlewares), req, res, ctx, static_cast<parent_context_t&>(ctx));
	}
	template <int N, typename Context, typename Container>
	typename std::enable_if<(N > 0)>::type after_handlers_call_helper(Container& middlewares, Context& ctx, Req& req, Res& res) {
	  using parent_context_t = typename Context::template partial<N - 1>;
	  using CurrentMW = typename std::tuple_element<N, typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW, Context, parent_context_t>(std::get<N>(middlewares), req, res, ctx, static_cast<parent_context_t&>(ctx));
	  after_handlers_call_helper<N - 1, Context, Container>(middlewares, ctx, req, res);
	}
  }
  using namespace boost;
  using tcp = asio::ip::tcp;
  /// An HTTP connection.
  template <typename Adaptor, typename Handler, typename ... Middlewares>
  class Connection : http_parser {
	friend struct cc::Res;
  public:
	Connection(boost::asio::io_service& io_service,
	  Handler* handler, std::tuple<Middlewares...>* middlewares,
	  std::function<std::string()>& get_cached_date_str_f,
	  detail::dumb_timer_queue& timer_queue,
	  typename Adaptor::Ctx* adaptor_ctx_,
	  std::atomic<uint16_t>& queue_length) :
	  adaptor_(io_service, adaptor_ctx_),
	  handler_(handler),
	  middlewares_(middlewares),
	  get_cached_date_str(get_cached_date_str_f),
	  timer_queue_(timer_queue),
	  queue_length_(queue_length) {
	  llhttp_init(this, HTTP_REQUEST, &settings_);
	}
	~Connection() {
	  cancel_deadline_timer();// res.complete_request_handler_ = nullptr;
	}
	static int on_url(http_parser* self_, const char* at, size_t length) {
	  Connection* $ = static_cast<Connection*>(self_);
	  $->header_state = 0; $->url.clear(); $->raw_url.clear(); $->header_field.clear();
	  $->header_value.clear(); $->headers.clear(); $->url_params.clear(); $->body.clear();
	  $->raw_url.insert($->raw_url.end(), at, at + length); return 0;
	}
	static int on_header_field(http_parser* self_, const char* at, size_t length) {
	  Connection* $ = static_cast<Connection*>(self_);
	  switch ($->header_state) {
	  case 0:if (!$->header_value.empty()) $->headers.emplace(std::move($->header_field.c_str()), std::move($->header_value));
		$->header_field.assign(at, at + length); $->header_state = 1; break;
	  case 1:$->header_field.insert($->header_field.end(), at, at + length); break;
	  } return 0;
	}
	static int on_header_value(http_parser* self_, const char* at, size_t length) {
	  Connection* $ = static_cast<Connection*>(self_);
	  switch ($->header_state) {
	  case 0:$->header_value.insert($->header_value.end(), at, at + length); break;
	  case 1:$->header_state = 0; $->header_value.assign(at, at + length); break;
	  } return 0;
	}
	static int on_headers_complete(http_parser* self_) {
	  Connection* $ = static_cast<Connection*>(self_);
	  if (!$->header_field.empty()) $->headers.emplace(std::move($->header_field), std::move($->header_value));
	  //always HTTP 1.1 Expect: 100-continue
	  if ($->http_minor == 1 && $->headers.count("expect") && get_header_value($->headers, "expect") == "100-continue") { $->buffers_.clear(); $->buffers_ += Res_expect; $->do_write(); }
	  return 0;
	}
	static int on_body(http_parser* self_, const char* at, size_t length) {
	  Connection* $ = static_cast<Connection*>(self_); $->body.insert($->body.end(), at, at + length); return 0;
	}
	static int on_message_complete(http_parser* self_) {
	  Connection* $ = static_cast<Connection*>(self_); $->url = $->raw_url.substr(0, $->raw_url.find("?"));
	  $->url_params = query_string($->raw_url); $->handle(); return 0;
	}
	Req to_request() const {
	  return Req{ static_cast<HTTP>(method), std::move(raw_url), std::move(url), std::move(url_params), std::move(headers), std::move(body) };
	}
	decltype(std::declval<Adaptor>().raw_socket())& socket() { return adaptor_.raw_socket(); }
	inline void start() {
	  adaptor_.start([this](const boost::system::error_code& ec) {
		if (!ec) {
		  start_deadline(); do_read();
		} else {
		  adaptor_.close(); delete this;
		}
		});
	}

	inline void handle() {
	  cancel_deadline_timer();
	  buffers_.clear();
	  bool is_invalid_request = false;
	  req_ = std::move(to_request());//http_major == 1 [Always]
	  req_.remoteIpAddress = adaptor_.remote_endpoint().address().to_string();
	  if (http_minor == 0) {// HTTP/1.0
		close_connection_ = true;
	  } else if (http_minor == 1) {// HTTP/1.1
		if (req_.headers.count("Connection") && req_.get_header_value("Connection") == "close") { close_connection_ = true; }
		if (!req_.headers.count("host")) { is_invalid_request = true; res = Res(400); }
		if (upgrade) {
		  if (req_.get_header_value("upgrade") == "h2c") {
			// TODO HTTP/2 currently, ignore upgrade header
		  } else {
			close_connection_ = true;
			handler_->handle_upgrade(req_, res, std::move(adaptor_));
			return;
		  }
		}
	  }
	  need_to_call_after_handlers_ = false;
	  if (req_.method == HTTP::OPTIONS) { res.code = 204; res.end(); complete_request(); } else if (!is_invalid_request) {
		res.is_alive_helper_ = [this]()->bool { return adaptor_.is_open(); };
		ctx_ = detail::Ctx<Middlewares...>();
		req_.middleware_context = static_cast<void*>(&ctx_);
		req_.io_service = &adaptor_.get_io_service();
		if (!res.completed_) {
		  res.complete_request_handler_ = [this] { complete_request(); };
		  need_to_call_after_handlers_ = true;
		  handler_->handle(req_, res);
		} else {
		  res.complete_request_handler_ = [] {};
		  complete_request();
		}
	  } else {
		complete_request();
	  }
	}
	/// Call the after handle middleware and send the write the Res to the connection.
	inline void complete_request() {
	  LOG_INFO << "Response: " << this << ' ' << req_.raw_url << ' ' << res.code << ' ' << close_connection_;
	  if (need_to_call_after_handlers_) {
		need_to_call_after_handlers_ = false;
		// call all after_handler of middlewares
		detail::after_handlers_call_helper<
		  (static_cast<int>(sizeof...(Middlewares)) - 1),
		  decltype(ctx_),
		  decltype(*middlewares_)>
		  (*middlewares_, ctx_, req_, res);
	  }
	  uint8_t num_headers_ = 2;
#ifdef AccessControlAllowCredentials
	  ++num_headers_;
#endif
#ifdef AccessControlAllowHeaders
	  ++num_headers_;
#endif
#ifdef AccessControlAllowMethods
	  ++num_headers_;
#endif
#ifdef AccessControlAllowOrigin
	  ++num_headers_;
#endif
	  set_status(res.code);
	  if (res.is_file) {
		buffers_.reserve(4 * (res.headers.size() + num_headers_) + 7);
		prepare_buffers();
		buffers_ += Res_Ca;
		buffers_ += Res_seperator;
		buffers_ += FILE_TIME;
		buffers_ += Res_crlf;
		buffers_ += RES_Xc;
		buffers_ += Res_seperator;
		buffers_ += RES_No;
		buffers_ += Res_crlf;
		buffers_ += Res_crlf;
		do_write_static();
	  } else {
		buffers_.reserve(4 * (res.headers.size() + num_headers_) + 15);
		prepare_buffers();
		if (!res.headers.count(RES_CT)) {
		  buffers_ += RES_CT;
		  buffers_ += Res_seperator;
		  buffers_ += RES_Txt;
		  buffers_ += Res_crlf;
		}
		detail::middleware_call_helper<0, decltype(ctx_), decltype(*middlewares_), Middlewares...>(*middlewares_, req_, res, ctx_);
		buffers_ += Res_content_length_tag;
		buffers_ += std::to_string(res.body.size());
		buffers_ += Res_crlf;
#if SHOW_SERVER_NAME
		buffers_ += Res_server_tag;
		buffers_ += SERVER_NAME;
		buffers_ += Res_crlf;
#endif
		date_str_ = get_cached_date_str();
		buffers_ += Res_date_tag;
		buffers_ += date_str_;
		buffers_ += Res_crlf;
		buffers_ += Res_crlf;
#ifdef ENABLE_COMPRESSION
		std::string accept_encoding = req_.get_header_value("Accept-Encoding");
		if (!accept_encoding.empty() && res.compressed) {
		  switch (handler_->compression_algorithm()) {
		  case compression::DEFLATE:
			if (accept_encoding.find("deflate") != std::string::npos) {
			  res.body = compression::compress_string(res.body, compression::algorithm::DEFLATE);
			  res.set_header("Content-Encoding", "deflate");
			}
			break;
		  case compression::GZIP:
			if (accept_encoding.find("gzip") != std::string::npos) {
			  res.body = compression::compress_string(res.body, compression::algorithm::GZIP);
			  res.set_header("Content-Encoding", "gzip");
			}
			break;
		  default:
			break;
		  }
		}
#endif
		do_write_general();
	  }
	  //if there is a redirection with a partial URL, treat the URL as a route.
	  std::string location = res.get_header_value("Location");
	  if (!location.empty() && location.find("://", 0) == std::string::npos) {
#ifdef ENABLE_SSL
		location.insert(0, "https://" + req_.get_header_value("Host"));
#else
		location.insert(0, "http://" + req_.get_header_value("Host"));
#endif
		res.add_header(Res_loc, location);
	  }
	}

  private:
	inline void set_status(uint16_t status) {
	  res.code = status;
	  switch (status) {
	  case 200:status_ = "200 OK\r\n"; break;
	  case 201:status_ = "201 Created\r\n"; break;
	  case 202:status_ = "202 Accepted\r\n"; break;
	  case 203:status_ = "203 Non-Authoritative Information\r\n"; break;
	  case 204:status_ = "204 No Content\r\n"; break;

	  case 301:status_ = "301 Moved Permanently\r\n"; break;
	  case 302:status_ = "302 Found\r\n"; break;
	  case 303:status_ = "303 See Other\r\n"; break;
	  case 304:status_ = "304 Not Modified\r\n"; break;
	  case 307:status_ = "307 Temporary redirect\r\n"; break;

	  case 400:status_ = "400 Bad Request\r\n"; res.body = status_; break;
	  case 401:status_ = "401 Unauthorized\r\n"; res.body = status_; break;
	  case 402:status_ = "402 Payment Required\r\n"; res.body = status_; break;
	  case 403:status_ = "403 Forbidden\r\n"; res.body = status_; break;
	  case 405:status_ = "405 HTTP verb used to access this page is not allowed\r\n"; res.body = status_; break;
	  case 406:status_ = "406 Browser does not accept the MIME type of the requested page\r\n"; res.body = status_; break;
	  case 409:status_ = "409 Conflict\r\n"; res.body = status_; break;

	  case 500:status_ = "500 Internal Server Error\r\n"; break;
	  case 501:status_ = "501 Not Implemented\r\n"; res.body = status_; break;
	  case 502:status_ = "502 Bad Gateway\r\n"; res.body = status_; break;
	  case 503:status_ = "503 Service Unavailable\r\n"; res.body = status_; break;

	  default:status_ = "404 Not Found\r\n"; res.body = status_; break;
	  }
	}
	inline void prepare_buffers() {
	  res.complete_request_handler_ = nullptr;
	  buffers_ += Res_http_status;
	  buffers_ += status_;
	  //if (res.code > 399) res.body = status_;
	  for (auto& kv : res.headers) {
		buffers_ += kv.first;
		buffers_ += Res_seperator;
		buffers_ += kv.second;
		buffers_ += Res_crlf;
	  }
#ifdef AccessControlAllowCredentials
	  buffers_ += RES_AcC;
	  buffers_ += AccessControlAllowCredentials;
	  buffers_ += Res_crlf;
#endif
#ifdef AccessControlAllowHeaders
	  buffers_ += RES_AcH;
	  buffers_ += AccessControlAllowHeaders;
	  buffers_ += Res_crlf;
#endif
#ifdef AccessControlAllowMethods
	  buffers_ += RES_AcM;
	  buffers_ += AccessControlAllowMethods;
	  buffers_ += Res_crlf;
#endif
#ifdef AccessControlAllowOrigin
	  buffers_ += RES_AcO;
	  buffers_ += AccessControlAllowOrigin;
	  buffers_ += Res_crlf;
#endif
	}

	inline void do_write_static() {
	  boost::asio::write(adaptor_.socket(), boost::asio::buffer(buffers_));
	  if (res.statResult_ == 0) {
		std::ifstream is(res.path_.c_str(), std::ios::in | std::ios::binary);
		char buf[16384];
		while (is.read(buf, sizeof(buf)).gcount() > 0) {
		  std::vector<asio::const_buffer> buffers;
		  buffers.push_back(boost::asio::buffer(buf));
		  write_buffer_list(buffers);
		}
	  }
	  is_writing = false;
	  if (close_connection_) {
		adaptor_.shutdown_readwrite();
		adaptor_.close();
		check_destroy();
	  }
	  res.end();
	  res.clear();
	  buffers_.clear();
	}

	inline void do_write_general() {
	  if (res.body.length() < res_stream_threshold_) {
		buffers_ += res.body;
		do_write();
		if (need_to_start_read_after_complete_) {
		  need_to_start_read_after_complete_ = false;
		  start_deadline(); do_read();
		}
	  } else {
		is_writing = true;
		boost::asio::write(adaptor_.socket(), boost::asio::buffer(buffers_));
		if (body.length() > 0) {

		  std::string buf;
		  std::vector<boost::asio::const_buffer> buffers;
		  while (body.length() > 16384) {
			//buf.reserve(16385);
			buf = body.substr(0, 16384);
			body = body.substr(16384);
			buffers.clear();
			buffers.push_back(boost::asio::buffer(buf));
			write_buffer_list(buffers);
		  }
		  //Collect whatever is left (less than 16KB) and send it down the socket
		  //buf.reserve(is.length());
		  buf = body;
		  body.clear();
		  buffers.clear();
		  buffers.push_back(boost::asio::buffer(buf));
		  write_buffer_list(buffers);
		}
		is_writing = false;
		if (close_connection_) {
		  adaptor_.shutdown_readwrite();
		  adaptor_.close();
		  check_destroy();
		}
		res.end();
		res.clear();
		buffers_.clear();
	  }
	}

	inline void do_read() {
	  is_reading = true;
	  adaptor_.socket().async_read_some(boost::asio::buffer(buffer_),
		[this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
		  if (!ec) {
			if (llhttp_execute(this, buffer_.data(), bytes_transferred) == 0/* && adaptor_.is_open()*/) {
			  if (close_connection_) {
				cancel_deadline_timer();
				is_reading = false;
				check_destroy(); // adaptor will close after write
			  } else if (!need_to_call_after_handlers_) {
				start_deadline();
				do_read();
			  } else { // res will be completed later by user
				need_to_start_read_after_complete_ = true;
			  }
			}
		  } else {
			cancel_deadline_timer();
			adaptor_.shutdown_read();
			adaptor_.close();
			is_reading = false;
			check_destroy();
		  }
		});
	}

	inline void do_write() {
	  is_writing = true;
	  boost::asio::async_write(adaptor_.socket(), boost::asio::buffer(buffers_),
		[this](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
		  is_writing = false;
		  res.clear();
		  if (!ec) {
			if (close_connection_) {
			  adaptor_.shutdown_write();
			  adaptor_.close();
			  check_destroy();
			}
		  } else {
			//check_destroy();
			--queue_length_; delete this;
		  }
		});
	}
	inline void write_buffer_list(std::vector<boost::asio::const_buffer>& buffers) {
	  boost::asio::write(adaptor_.socket(), buffers, [this](std::error_code ec, std::size_t) {
		if (!ec) {
		  return false;
		} else {
		  LOG_ERROR << ec << " - happened while sending buffers";
		  check_destroy();
		  return true;
		}
		});
	}
	inline void check_destroy() { if (!is_reading && !is_writing) { --queue_length_; delete this; } }
	inline void cancel_deadline_timer() { timer_queue_.cancel(timer_cancel_key_); }
	inline void start_deadline(unsigned short timeout = cc::detail::dumb_timer_queue::tick) {
	  cancel_deadline_timer();
	  timer_cancel_key_ = timer_queue_.add([this] {
		if (!adaptor_.is_open()) { return; }
		adaptor_.shutdown_readwrite();
		adaptor_.close();
		}, timeout);
	}
  private:
	Adaptor adaptor_;
	Handler* handler_;

	boost::array<char, 4096> buffer_;
	const char* status_ = "404 Not Found\r\n";
	const unsigned res_stream_threshold_ = 1048576;

	inline static http_parser_settings settings_ = {
			nullptr,
			on_url,
			nullptr,
			on_header_field,
			on_header_value,
			on_headers_complete,
			on_body,
			on_message_complete,
	};
	std::string raw_url;
	std::string url;
	int header_state;
	std::string header_field;
	std::string header_value;
	ci_map headers;
	query_string url_params;
	detail::dumb_timer_queue& timer_queue_;
	uint16_t timer_cancel_key_;
	std::atomic<uint16_t>& queue_length_;
	Req req_;
	Res res;
	bool close_connection_ = false;
	std::string buffers_, body, date_str_;
	bool is_reading{};
	bool is_writing{};
	bool need_to_call_after_handlers_{};
	bool need_to_start_read_after_complete_{};
	std::tuple<Middlewares...>* middlewares_;
	detail::Ctx<Middlewares...> ctx_;
	std::function<std::string()>& get_cached_date_str;
  };
}
