#pragma once

#include <string>
#include <string_view>

#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include "async_nats/detail/capi.h"
#include "async_nats/subscribtion.hpp"
#include "async_nats/tokio_runtime.hpp"

namespace async_nats
{

class ConnectionErrorCategory : public boost::system::error_category
{
  // error_category interface
public:
  const char* name() const noexcept override { return "async_asio_connection_error"; }

  std::string message(int ev) const override
  {
    auto str = async_nats_io_error_description(ev);
    std::string message(str);
    async_nats_owned_string_delete(str);
    return message;
  }

  boost::system::error_condition default_error_condition(int ev) const noexcept override
  {
    auto code = async_nats_io_error_system_code(ev);
    if (!code.has_value) {
      return boost::system::error_condition(ev, *this);
    }
    return boost::system::error_condition(code.value, boost::system::system_category());
  }
};

inline const boost::system::error_category& zstd_error_category()
{
  static ConnectionErrorCategory instance;
  return instance;
}

inline boost::system::error_code make_error_code(int e)
{
  return boost::system::error_code(e, zstd_error_category());
}

inline boost::system::error_condition make_error_condition(int e)
{
  return boost::system::error_condition(e, zstd_error_category());
}

class ConnectionOptions
{
public:
  ConnectionOptions() { options_ = async_nats_connection_config_new(); }

  ConnectionOptions(const ConnectionOptions&) = delete;
  ConnectionOptions(ConnectionOptions&&) = default;

  ~ConnectionOptions() { async_nats_connection_config_delete(options_); }

  ConnectionOptions& name(const std::string& name)
  {
    async_nats_connection_config_name(options_, name.c_str());
    return *this;
  }

  ConnectionOptions& address(const std::string& addr)
  {
    async_nats_connection_config_addr(options_, addr.c_str());
    return *this;
  }

  AsyncNatsConnectionConfig* get_raw() { return options_; }

  const AsyncNatsConnectionConfig* get_raw() const { return options_; }

private:
  AsyncNatsConnectionConfig* options_;
};

class Connection
{
public:
  Connection() = default;

  Connection(AsyncNatsConnection* conn)
      : conn_(conn)
  {
  }

  Connection(const Connection& o) { conn_ = async_nats_connection_clone(o.get_raw()); }

  Connection(Connection&& o)
  {
    conn_ = o.conn_;
    o.conn_ = nullptr;
  }

  ~Connection()
  {
    if (conn_) {
      async_nats_connection_delete(conn_);
    }
  }

  Connection& operator=(const Connection& o)
  {
    if (this == &o)
      return *this;

    if (conn_) {
      async_nats_connection_delete(conn_);
    }
    conn_ = async_nats_connection_clone(o.get_raw());

    return *this;
  }

  Connection& operator=(Connection&& o)
  {
    if (this == &o)
      return *this;

    if (conn_) {
      async_nats_connection_delete(conn_);
    }
    conn_ = o.conn_;
    o.conn_ = nullptr;

    return *this;
  }

  operator bool() const { return conn_ != nullptr; }

  AsyncNatsConnection* get_raw() { return conn_; }

  const AsyncNatsConnection* get_raw() const { return conn_; }

  /**
   * @brief publish
   * @param topic
   * @param data
   * @param token
   */
  template<class CompletionToken>
  auto publish(std::string_view topic, boost::asio::const_buffer data, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](void* ctx)
      {
        auto c = static_cast<CH*>(ctx);
        (*c)();
        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsPublishCallback cb {f, ctx};
      async_nats_connection_publish_async(
          get_raw(),
          topic.data(),
          {reinterpret_cast<const uint8_t*>(data.data()), data.size()},
          cb);
    };

    return boost::asio::async_initiate<CompletionToken, void()>(init, token);
  }

  template<class CompletionToken>
  auto subcribe(std::string_view topic, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsSubscription* sub, AsyncNatsOwnedString /*err*/, void* ctx)
      {
        auto c = static_cast<CH*>(ctx);
        (*c)(Subscribtion(sub));
        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsSubscribeCallback cb {f, ctx};
      async_nats_connection_subscribe_async(get_raw(), topic.data(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Subscribtion)>(init, token);
  }

private:
  AsyncNatsConnection* conn_ = nullptr;
};

template<class CompletionToken>
auto connect(const TokioRuntime& rt, const ConnectionOptions& options, CompletionToken&& token)
{
  auto init = [&](auto token)
  {
    using CH = std::decay_t<decltype(token)>;

    static auto f = [](AsyncNatsConnection* conn, AsyncNatsIoError e, void* ctx)
    {
      auto c = static_cast<CH*>(ctx);
      if (!conn) {
        (*c)(make_error_code(e), Connection(conn));
      } else {
        (*c)(boost::system::error_code(), Connection(conn));
      }

      delete c;
    };

    auto ctx = new CH(std::move(token));
    ::AsyncNatsConnectCallback cb {f, ctx};
    async_nats_connection_connect(rt.get_raw(), options.get_raw(), cb);
  };

  return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, Connection)>(
      init, token);
}

}  // namespace async_nats
