#pragma once
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <atomic>
#include <chrono>
#include <vector>

#include "crow/llhttp.h"
#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/settings.h"
#include "crow/detail.h"
#include "crow/middleware_context.h"
#include "crow/socket_adaptors.h"
#include "crow/compression.h"
static char Res_server_tag[9]="Server: ",Res_content_length_tag[17]="Content-Length: ",Res_http_status[10]="HTTP/1.1 ",
RES_AcC[35]="Access-Control-Allow-Credentials: ",RES_t[5]="true",RES_AcM[]="Access-Control-Allow-Methods: ",
RES_AcH[31]="Access-Control-Allow-Headers: ",RES_AcO[30]="Access-Control-Allow-Origin: ",
Res_date_tag[7]="Date: ",Res_content_length[15]="content-length",Res_seperator[3]=": ",Res_crlf[3]="\r\n",Res_loc[9]="location";
namespace crow {
  namespace detail {
	template <typename MW>struct check_before_handle_arity_3_const {
	  template <typename T,void(T::*)(Req&,Res&,typename MW::Ctx&)const=&T::before_handle >
	  struct get {};
	};
	template <typename MW>struct check_before_handle_arity_3 {
	  template <typename T,void(T::*)(Req&,Res&,typename MW::Ctx&)=&T::before_handle >
	  struct get {};
	};
	template <typename MW>struct check_after_handle_arity_3_const {
	  template <typename T,void(T::*)(Req&,Res&,typename MW::Ctx&)const=&T::after_handle >
	  struct get {};
	};
	template <typename MW>struct check_after_handle_arity_3 {
	  template <typename T,void(T::*)(Req&,Res&,typename MW::Ctx&)=&T::after_handle >
	  struct get {};
	};
	template <typename T>struct is_before_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3_const<T>::template get<C>*);
	  template <typename C>
	  static std::true_type f(typename check_before_handle_arity_3<T>::template get<C>*);
	  template <typename C>
	  static std::false_type f(...);
	  public: static const bool value=decltype(f<T>(nullptr))::value;
	};
	template <typename T>
	struct is_after_handle_arity_3_impl {
	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3_const<T>::template get<C>*);
	  template <typename C>
	  static std::true_type f(typename check_after_handle_arity_3<T>::template get<C>*);
	  template <typename C>
	  static std::false_type f(...);
	  public: static const bool value=decltype(f<T>(nullptr))::value;
	};
	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<!is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req,res,ctx.template get<MW>(),ctx);
	}
	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<is_before_handle_arity_3_impl<MW>::value>::type
	  before_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.before_handle(req,res,ctx.template get<MW>());
	}
	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<!is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req,res,ctx.template get<MW>(),ctx);
	}
	template <typename MW,typename Context,typename ParentContext>
	typename std::enable_if<is_after_handle_arity_3_impl<MW>::value>::type
	  after_handler_call(MW& mw,Req& req,Res& res,Context& ctx,ParentContext& /*parent_ctx*/) {
	  mw.after_handle(req,res,ctx.template get<MW>());
	}
	template <int N,typename Context,typename Container,typename CurrentMW,typename ... Middlewares>
	bool middleware_call_helper(Container& middlewares,Req& req,Res& res,Context& ctx) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  before_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	  if (res.is_completed()) {
		after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  if (middleware_call_helper<N+1,Context,Container,Middlewares...>(middlewares,req,res,ctx)) {
		after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
		return true;
	  }
	  return false;
	}
	template <int N,typename Context,typename Container>
	bool middleware_call_helper(Container& /*middlewares*/,Req& /*req*/,Res& /*res*/,Context& /*ctx*/) {
	  return false;
	}
	template <int N,typename Context,typename Container>
	typename std::enable_if<(N<0)>::type
	  after_handlers_call_helper(Container& /*middlewares*/,Context& /*context*/,Req& /*req*/,Res& /*res*/) {}
	template <int N,typename Context,typename Container>
	typename std::enable_if<(N==0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  using CurrentMW=typename std::tuple_element<N,typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	}
	template <int N,typename Context,typename Container>
	typename std::enable_if<(N>0)>::type after_handlers_call_helper(Container& middlewares,Context& ctx,Req& req,Res& res) {
	  using parent_context_t=typename Context::template partial<N-1>;
	  using CurrentMW=typename std::tuple_element<N,typename std::remove_reference<Container>::type>::type;
	  after_handler_call<CurrentMW,Context,parent_context_t>(std::get<N>(middlewares),req,res,ctx,static_cast<parent_context_t&>(ctx));
	  after_handlers_call_helper<N-1,Context,Container>(middlewares,ctx,req,res);
	}
  }
  using namespace boost;
  using tcp=asio::ip::tcp;
  /// An HTTP connection.
  template <typename Adaptor,typename Handler,typename ... Middlewares>
  class Connection : http_parser {
	friend struct crow::Res;
	public:
	Connection(boost::asio::io_service& io_service,
	  Handler* handler,const std::string& server_name,
	  std::tuple<Middlewares...>* middlewares,
	  std::function<std::string()>& get_cached_date_str_f,
	  detail::dumb_timer_queue& timer_queue,
	  typename Adaptor::Ctx* adaptor_ctx_):
	  adaptor_(io_service,adaptor_ctx_),
	  handler_(handler),
	  server_name_(server_name),
	  middlewares_(middlewares),
	  get_cached_date_str(get_cached_date_str_f),
	  timer_queue_(timer_queue) {
	  llhttp_init(this,HTTP_REQUEST,&settings_);
	}
	~Connection() { res.complete_request_handler_=nullptr;cancel_deadline_timer();}
	// return false on error
	bool feed(const char* buffer,int length) { return llhttp_execute(this,buffer,length)==0; }
	static int on_url(http_parser* self_,const char* at,size_t length) {
	  Connection* self=static_cast<Connection*>(self_);
	  self->header_state = 0;self->url.clear();self->raw_url.clear();self->header_field.clear();
	  self->header_value.clear();self->headers.clear();self->url_params.clear();self->body.clear();
	  self->raw_url.insert(self->raw_url.end(),at,at+length);
	  return 0;
	}
	static int on_header_field(http_parser* self_,const char* at,size_t length) {
	  Connection* self=static_cast<Connection*>(self_);
	  switch (self->header_state) {
		case 0:if (!self->header_value.empty()) self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
		  self->header_field.assign(at,at+length);self->header_state=1;break;
		case 1:self->header_field.insert(self->header_field.end(),at,at+length);break;
	  }
	  return 0;
	}
	static int on_header_value(http_parser* self_,const char* at,size_t length) {
	  Connection* self=static_cast<Connection*>(self_);
	  switch (self->header_state) {
		case 0:self->header_value.insert(self->header_value.end(),at,at+length);break;
		case 1:self->header_state=0;self->header_value.assign(at,at+length);break;
	  }
	  return 0;
	}
	static int on_headers_complete(http_parser* self_) {
	  Connection* self=static_cast<Connection*>(self_);
	  if (!self->header_field.empty()) self->headers.emplace(std::move(self->header_field),std::move(self->header_value));
	  // HTTP 1.1 Expect: 100-continue
	  if (self->http_major==1&&self->http_minor==1&&self->headers.count("expect")&&get_header_value(self->headers,"expect")=="100-continue") {
		self->buffers_.clear();static std::string expect_100_continue="HTTP/1.1 100 Continue\r\n\r\n";
		self->buffers_.emplace_back(expect_100_continue.data(),expect_100_continue.size());self->do_write();
	  }
	  return 0;
	}
	static int on_body(http_parser* self_,const char* at,size_t length) {
	  Connection* self=static_cast<Connection*>(self_);self->body.insert(self->body.end(),at,at+length);
	  return 0;
	}
	static int on_message_complete(http_parser* self_) {
	  Connection* self=static_cast<Connection*>(self_);self->url=self->raw_url.substr(0,self->raw_url.find("?"));
	  self->url_params=query_string(self->raw_url);self->handle();
	  return 0;
	}
	Req to_request() const {
	  //printf("this:raw_url: %s\n",this->raw_url.data());printf("this:method: %d\n",this->method);
	  return Req{static_cast<HTTPMethod>(this->method), std::move(this->raw_url), std::move(this->url), std::move(this->url_params), std::move(this->headers), std::move(this->body)};
	}
	bool is_upgrade() const { return this->upgrade; }
	/// The TCP socket on top of which the connection is established.
	decltype(std::declval<Adaptor>().raw_socket())& socket() { return adaptor_.raw_socket(); }
	void start() {
	  adaptor_.start([this](const boost::system::error_code& ec) {
		if (!ec) {
		  start_deadline();do_read();
		} else delete this;
	  });
	}

	void handle() {
	  cancel_deadline_timer();
	  buffers_.clear();
	  bool is_invalid_request=false;
	  req_=std::move(to_request());
	  req_.remoteIpAddress=adaptor_.remote_endpoint().address().to_string();
	  if (this->http_major==1&&this->http_minor==0) {// HTTP/1.0
		close_connection_=true;
	  } else if (this->http_major==1&&this->http_minor==1) {// HTTP/1.1
		if (req_.headers.count("Connection")&&req_.get_header_value("Connection")=="close") close_connection_=true;
		if (!req_.headers.count("host")) {
		  is_invalid_request=true;res=Res(400);
		}
		if (is_upgrade()) {
		  if (req_.get_header_value("upgrade")=="h2c") {
			// TODO HTTP/2 currently, ignore upgrade header
		  } else {
			close_connection_=true;
			handler_->handle_upgrade(req_,res,std::move(adaptor_));
			return;
		  }
		}
	  }
	  CROW_LOG_INFO<<"Request: "<<boost::lexical_cast<std::string>(adaptor_.remote_endpoint())<<" "<<this<<" HTTP/"<<this->http_major<<"."<<this->http_minor<<' '
		<<m2s(req_.method)<<" "<<req_.url;
	  need_to_call_after_handlers_=false;
	  if (req_.method==HTTPMethod::OPTIONS) { res.code=204;res.end();complete_request(); } else if (!is_invalid_request) {
		res.complete_request_handler_=[] {};
		//res.is_alive_helper_=[this]()->bool { return adaptor_.is_open(); };

		ctx_=detail::Ctx<Middlewares...>();
		req_.middleware_context=static_cast<void*>(&ctx_);
		req_.io_service=&adaptor_.get_io_service();
		if (!res.completed_) {
		  res.complete_request_handler_=[this] { this->complete_request(); };
		  need_to_call_after_handlers_=true;
		  handler_->handle(req_,res);
		} else {
		  complete_request();
		}
	  } else {
		complete_request();
	  }
	}
	/// Call the after handle middleware and send the write the Res to the connection.
	void complete_request() {
	  if (!adaptor_.is_open()) {
		res.complete_request_handler_=nullptr;//delete this;
		return;
	  }
	  CROW_LOG_INFO<<"Response: "<<this<<' '<<req_.raw_url<<' '<<res.code<<' '<<close_connection_;
	  if (need_to_call_after_handlers_) {
		need_to_call_after_handlers_=false;
		// call all after_handler of middlewares
		detail::after_handlers_call_helper<
		  (static_cast<int>(sizeof...(Middlewares))-1),
		  decltype(ctx_),
		  decltype(*middlewares_)>
		  (*middlewares_,ctx_,req_,res);
	  }//res.complete_request_handler_=nullptr;
	  num_headers_=1;
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
		buffers_.reserve(4*(res.headers.size()+num_headers_)+3);
		prepare_buffers();
		buffers_.emplace_back(Res_crlf,2);
		do_write_static();
	  } else {
		buffers_.reserve(4*(res.headers.size()+num_headers_)+15);
		prepare_buffers();
		detail::middleware_call_helper<0,decltype(ctx_),decltype(*middlewares_),Middlewares...>(*middlewares_,req_,res,ctx_);
		hack_=std::to_string(res.body.size());
		buffers_.emplace_back(Res_content_length_tag,16);
		buffers_.emplace_back(hack_.data(),hack_.size());
		buffers_.emplace_back(Res_crlf,2);

		buffers_.emplace_back(Res_server_tag,8);
		buffers_.emplace_back(server_name_.data(),server_name_.size());
		buffers_.emplace_back(Res_crlf,2);

		date_str_=get_cached_date_str();
		buffers_.emplace_back(Res_date_tag,6);
		buffers_.emplace_back(date_str_.data(),date_str_.size());
		buffers_.emplace_back(Res_crlf,2);
		buffers_.emplace_back(Res_crlf,2);
#ifdef CROW_ENABLE_COMPRESSION
		std::string accept_encoding=req_.get_header_value("Accept-Encoding");
		if (!accept_encoding.empty()&&res.compressed) {
		  switch (handler_->compression_algorithm()) {
			case compression::DEFLATE:
			if (accept_encoding.find("deflate")!=std::string::npos) {
			  res.body=compression::compress_string(res.body,compression::algorithm::DEFLATE);
			  res.set_header("Content-Encoding","deflate");
			}
			break;
			case compression::GZIP:
			if (accept_encoding.find("gzip")!=std::string::npos) {
			  res.body=compression::compress_string(res.body,compression::algorithm::GZIP);
			  res.set_header("Content-Encoding","gzip");
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
	  std::string location=res.get_header_value("Location");
	  if (!location.empty()&&location.find("://",0)==std::string::npos) {
#ifdef CROW_ENABLE_SSL
		location.insert(0,"https://"+req_.get_header_value("Host"));
#else
		location.insert(0,"http://"+req_.get_header_value("Host"));
#endif
		res.add_header_t(Res_loc,location);
	  }
	}

	private:
	void set_status(int status) {
	  res.code=status;
	  switch (status) {
		case 200:status_="200 OK\r\n",status_len_=8;break;
		case 201:status_="201 Created\r\n",status_len_=13;break;
		case 202:status_="202 Accepted\r\n",status_len_=14;break;
		case 203:status_="203 Non-Authoritative Information\r\n",status_len_=35;break;
		case 204:status_="204 No Content\r\n",status_len_=16;break;

		case 301:status_="301 Moved Permanently\r\n",status_len_=23;break;
		case 302:status_="302 Found\r\n",status_len_=11;break;
		case 303:status_="303 See Other\r\n",status_len_=15;break;
		case 304:status_="304 Not Modified\r\n",status_len_=18;break;
		case 307:status_="307 Temporary redirect\r\n",status_len_=24;break;

		case 400:status_="400 Bad Request\r\n",status_len_=17;break;
		case 401:status_="401 Unauthorized\r\n",status_len_=19;break;
		case 402:status_="402 Payment Required\r\n",status_len_=22;break;
		case 403:status_="403 Forbidden\r\n",status_len_=15;break;
		case 405:status_="405 HTTP verb used to access this page is not allowed (method not allowed)\r\n",status_len_=76;break;
		case 406:status_="406 Client browser does not accept the MIME type of the requested page\r\n",status_len_=72;break;
		case 409:status_="409 Conflict\r\n",status_len_=14;break;

		case 500:status_="500 Internal Server Error\r\n",status_len_=27;break;
		case 501:status_="501 Not Implemented\r\n",status_len_=21;break;
		case 502:status_="502 Bad Gateway\r\n",status_len_=17;break;
		case 503:status_="503 Service Unavailable\r\n",status_len_=25;break;

		default:status_="404 Not Found\r\n",status_len_=15;break;
	  }
	}
	void prepare_buffers() {
	  //if (res.body.empty()) {}//res.body
	  //res.complete_request_handler_=nullptr;
	  buffers_.emplace_back(Res_http_status,9);
	  buffers_.emplace_back(status_,status_len_);
	  if (res.code>399) res.body=status_;
	  for (auto& kv:res.headers) {
		buffers_.emplace_back(kv.first.data(),kv.first.size());
		buffers_.emplace_back(Res_seperator,2);
		buffers_.emplace_back(kv.second.data(),kv.second.size());
		buffers_.emplace_back(Res_crlf,2);
	  }
#ifdef AccessControlAllowCredentials
	  buffers_.emplace_back(RES_AcC,34);
	  buffers_.emplace_back(AccessControlAllowCredentials,ACAC);
	  buffers_.emplace_back(Res_crlf,2);
#endif
#ifdef AccessControlAllowHeaders
	  buffers_.emplace_back(RES_AcH,30);
	  buffers_.emplace_back(AccessControlAllowHeaders,ACAH);
	  buffers_.emplace_back(Res_crlf,2);
#endif
#ifdef AccessControlAllowMethods
	  buffers_.emplace_back(RES_AcM,30);
	  buffers_.emplace_back(AccessControlAllowMethods,ACAM);
	  buffers_.emplace_back(Res_crlf,2);
#endif
#ifdef AccessControlAllowOrigin
	  buffers_.emplace_back(RES_AcO,29);
	  buffers_.emplace_back(AccessControlAllowOrigin,ACAO);
	  buffers_.emplace_back(Res_crlf,2);
#endif
	}

	void do_write_static() {
	  boost::asio::write(adaptor_.socket(),buffers_);
	  res.do_stream_file(adaptor_);
	  res.end();
	  res.clear();
	  buffers_.clear();
	}

	void do_write_general() {
	  if (res.body.length()<res_stream_threshold_) {
		buffers_.emplace_back(res.body.data(),res.body.size());
		do_write();
		if (need_to_start_read_after_complete_) {
		  need_to_start_read_after_complete_=false;start_deadline();do_read();
		}
	  } else {
		is_writing=true;
		boost::asio::write(adaptor_.socket(),buffers_);
		res.do_stream_body(adaptor_);
		res.end();
		res.clear();
		buffers_.clear();
	  }
	}

	void do_read() {
	  //auto self = this->shared_from_this();
	  is_reading=true;
	  adaptor_.socket().async_read_some(boost::asio::buffer(buffer_),
	[this](const boost::system::error_code& ec,std::size_t bytes_transferred) {
		bool error_while_reading = true;
		if (!ec) {
		  bool ret = feed(buffer_.data(),bytes_transferred);
		  if (ret&&adaptor_.is_open()) error_while_reading = false;
		}
		if (error_while_reading) {
		  cancel_deadline_timer();
		  adaptor_.shutdown_read();
		  adaptor_.close();
		  is_reading = false;
		  check_destroy();
		} else if (close_connection_) {
		  cancel_deadline_timer();
		  is_reading = false;
		  //check_destroy();
		  delete this;
		} else if (!need_to_call_after_handlers_) {// adaptor will close after write
		  start_deadline();do_read();
		} else {// res will be completed later by user
		  need_to_start_read_after_complete_ = true;
		}
	  });
	}

	void do_write() {
	  //auto self = this->shared_from_this();
	  is_writing=true;
	  boost::asio::async_write(adaptor_.socket(),buffers_,
							   [this](const boost::system::error_code& ec,std::size_t /*bytes_transferred*/) {
		is_writing=false;
		res.clear();
		if (!ec) {
		  if (close_connection_) {
			adaptor_.shutdown_write();
			adaptor_.close();
			check_destroy();
		  }
		} else {
		  delete this;
		}
	  });
	}

	void check_destroy() {
	  CROW_LOG_DEBUG<<this<<" is_reading "<<is_reading<<" is_writing "<<is_writing;
	  if (!is_reading&&!is_writing) {
		CROW_LOG_DEBUG<<this<<" delete (idle) ";
		delete this;
	  }
	}
	void cancel_deadline_timer() {
	  CROW_LOG_DEBUG<<this<<" timer cancelled: "<<timer_cancel_key_.first<<' '<<timer_cancel_key_.second;
	  timer_queue_.cancel(timer_cancel_key_);
	}

	void start_deadline(/*int timeout = 4*/) {
	  timer_queue_.cancel(timer_cancel_key_);
	  timer_cancel_key_=timer_queue_.add([this] {
		if (adaptor_.is_open()) {
		  adaptor_.shutdown_readwrite();
		  adaptor_.close();
		}
	  });
	  CROW_LOG_DEBUG<<this<<" timer added: "<<timer_cancel_key_.first<<' '<<timer_cancel_key_.second;
	}
	private:
	Adaptor adaptor_;
	Handler* handler_;

	boost::array<char,4096> buffer_;
	const char* status_="404 Not Found\r\n";
	int status_len_=15;
	const unsigned res_stream_threshold_=1048576;
	uint8_t num_headers_;

	//std::unique_ptr<http_parser> parser_;
	const http_parser_settings settings_={
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
	std::string body;
	detail::dumb_timer_queue& timer_queue_;
	detail::dumb_timer_queue::key timer_cancel_key_;

	Req req_;
	Res res;

	bool close_connection_=false;
	const std::string& server_name_;
	std::vector<boost::asio::const_buffer> buffers_;

	std::string hack_,date_str_;
	bool is_reading{};
	bool is_writing{};
	bool need_to_call_after_handlers_{};
	bool need_to_start_read_after_complete_{};
	std::tuple<Middlewares...>* middlewares_;
	detail::Ctx<Middlewares...> ctx_;
	std::function<std::string()>& get_cached_date_str;
  };
}
