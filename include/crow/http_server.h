#pragma once
#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include <cstdint>
#include <atomic>
#include <future>
#include <vector>
#include <memory>
#include "crow/http_connection.h"
#include "crow/logging.h"
#include "crow/detail.h"
namespace boost {
  namespace posix_time {
    class BOOST_SYMBOL_VISIBLE millseconds : public time_duration {
      public:template <typename T>
        BOOST_CXX14_CONSTEXPR explicit millseconds(T const& s,
             typename boost::enable_if<boost::is_integral<T>,void>::type* =BOOST_DATE_TIME_NULLPTR):
        time_duration(0,0,0,numeric_cast<fractional_seconds_type>(s)) {}
    };
  }
}
namespace crow {
  static const char RES_GMT[26]="%a, %d %b %Y %H:%M:%S GMT";
  using namespace boost;
  using tcp=asio::ip::tcp;
  template <typename Handler,typename Adaptor=SocketAdaptor,typename ... Middlewares>
  class Server {
    public:
    Server(Handler* handler,std::string bindaddr,uint16_t port,std::string server_name,std::tuple<Middlewares...>* middlewares=nullptr,uint16_t concurrency=1,typename Adaptor::Ctx* adaptor_ctx=nullptr)
      : acceptor_(io_service_,tcp::endpoint(boost::asio::ip::address::from_string(bindaddr),port)),
      signals_(io_service_,SIGINT,SIGTERM),
      tick_timer_(io_service_),
      handler_(handler),
      concurrency_(concurrency==0?1:concurrency),
      server_name_(server_name),
      port_(port),
      bindaddr_(bindaddr),
      middlewares_(middlewares),
      adaptor_ctx_(adaptor_ctx) {}

    void set_tick_function(std::chrono::milliseconds d,std::function<void()> f) {
      tick_interval_=d;
      tick_function_=f;
    }
    void run() {
      for (int i=0; i<concurrency_; ++i)
        io_service_pool_.emplace_back(new boost::asio::io_service());
      get_cached_date_str_pool_.resize(concurrency_);
      timer_queue_pool_.resize(concurrency_);

      std::vector<std::future<void>> v;
      std::atomic<int> init_count(0);
      for (uint16_t i=0; i<concurrency_; ++i)
        v.push_back(
          std::async(std::launch::async,[this,i,&init_count] {
        // thread local date string get function
        auto last=std::chrono::steady_clock::now();
        std::string date_str;date_str.resize(0x20);
        get_cached_date_str_pool_[i]=[&date_str,&last]()->std::string {
          if (std::chrono::steady_clock::now()-last>std::chrono::seconds(2)) {
            time_t last_time_t=time(0);tm my_tm;
            last=std::chrono::steady_clock::now();
#if defined(_MSC_VER) || defined(__MINGW32__)
            localtime_s(&my_tm,&last_time_t);
#else
            localtime_r(&last_time_t,&my_tm);
#endif
            date_str.resize(strftime(&date_str[0],0x1f,RES_GMT,&my_tm));
          }
          return date_str;
        };
        // initializing timer queue
        detail::dumb_timer_queue timer_queue;
        timer_queue_pool_[i]=&timer_queue;
        timer_queue.set_io_service(*io_service_pool_[i]);
        boost::asio::deadline_timer timer(*io_service_pool_[i]);
        std::function<void(const boost::system::error_code&)> handler;
        timer.expires_from_now(boost::posix_time::millseconds(1));
        timer.async_wait(handler=[&timer_queue,&timer,&handler](const boost::system::error_code& /*ec*/) {
          //if (ec)return;//asciphx
          timer_queue.process();
          timer.expires_from_now(boost::posix_time::millseconds(1));
          timer.async_wait(handler);
        });
        ++init_count;
        io_service_pool_[i]->run();
      }));

      //if (/*tick_function_&&*/tick_interval_.count()>0) {
      //}
      CROW_LOG_INFO<<server_name_<<" server is running at "<<bindaddr_<<":"<<acceptor_.local_endpoint().port()
        <<" using "<<concurrency_<<" threads";
      CROW_LOG_INFO<<"Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs.";

      signals_.async_wait(
        [this](const boost::system::error_code& /*error*/,int /*signal_number*/) {
        stop();
      });

      while (concurrency_!=init_count) std::this_thread::yield();
      do_accept();
      std::thread([this] {
        io_service_.run();
        CROW_LOG_INFO<<"Exiting.";
      }).join();
      while (tick_interval_.count()>0) {
        tick_timer_.expires_from_now(boost::posix_time::millseconds(tick_interval_.count()));
        tick_timer_.async_wait([this](const boost::system::error_code& /*ec*/) {
          //if (ec) return;
          tick_function_();
        });
      }
    }

    void stop() {
      io_service_.stop();
      for (auto& io_service:io_service_pool_) io_service->stop();
    }

    void signal_clear() { signals_.clear(); }
    void signal_add(int signal_number) { signals_.add(signal_number); }

    private:
    asio::io_service& pick_io_service() {
      // TODO load balancing
      ++roundrobin_index_;
      if (roundrobin_index_>=io_service_pool_.size()) roundrobin_index_=0;
      return *io_service_pool_[roundrobin_index_];
    }

    void do_accept() {
      asio::io_service& is=pick_io_service();
      auto p=new Connection<Adaptor,Handler,Middlewares...>(
        is,handler_,server_name_,middlewares_,
        get_cached_date_str_pool_[roundrobin_index_],*timer_queue_pool_[roundrobin_index_],
        adaptor_ctx_);
      acceptor_.async_accept(p->socket(),
        [this,p,&is](const boost::system::error_code&ec) {
        if (!ec) {
          is.post([p] { p->start(); });
        } else
          delete p;
        do_accept();
      });
    }

    private:
    asio::io_service io_service_;
    std::vector<std::unique_ptr<asio::io_service>> io_service_pool_;
    std::vector<detail::dumb_timer_queue*> timer_queue_pool_;
    std::vector<std::function<std::string()>> get_cached_date_str_pool_;
    tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    boost::asio::deadline_timer tick_timer_;

    Handler* handler_;
    uint16_t concurrency_{1};
    std::string server_name_;
    uint16_t port_;
    std::string bindaddr_;
    unsigned int roundrobin_index_{};

    std::chrono::milliseconds tick_interval_;
    std::function<void()> tick_function_;

    std::tuple<Middlewares...>* middlewares_;

#ifdef CROW_ENABLE_SSL
    bool use_ssl_{false};
    boost::asio::ssl::context ssl_context_{boost::asio::ssl::context::sslv23};
#endif
    typename Adaptor::Ctx* adaptor_ctx_;
  };
}
