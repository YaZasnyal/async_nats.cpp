#pragma once

#include <string>
#include <string_view>

#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include <async_nats/detail/owned_string.h>
#include <async_nats/subscribtion.hpp>
#include <async_nats/tokio_runtime.hpp>

namespace async_nats
{
class ConnectionOptions
{
public:
  ConnectionOptions()
  {
    options_ = async_nats_connection_config_new();
  }

  ConnectionOptions(const ConnectionOptions&) = delete;
  ConnectionOptions(ConnectionOptions&&) = default;

  ~ConnectionOptions()
  {
    async_nats_connection_config_delete(options_);
  }

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

  AsyncNatsConnetionParams* get_raw()
  {
    return options_;
  }

  const AsyncNatsConnetionParams* get_raw() const
  {
    return options_;
  }

private:
  AsyncNatsConnetionParams* options_;
};

class Connection
{
public:
  Connection() = default;

  Connection(AsyncNatsConnection* conn)
      : conn_(conn)
  {
  }

  Connection(const Connection& o)
  {
    conn_ = async_nats_connection_clone(o.get_raw());
  }

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

  operator bool() const
  {
    return conn_ != nullptr;
  }

  AsyncNatsConnection* get_raw()
  {
    return conn_;
  }

  const AsyncNatsConnection* get_raw() const
  {
    return conn_;
  }

  /**
   * @brief new_mailbox function generates new random string that can be used for replies
   */
  detail::OwnedString new_mailbox() const
  {
    return detail::OwnedString(async_nats_connection_mailbox(conn_));
  }

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
          get_raw(), topic.data(), {reinterpret_cast<const char*>(data.data()), data.size()}, cb);
    };

    return boost::asio::async_initiate<CompletionToken, void()>(init, token);
  }

  template<class CompletionToken>
  auto subcribe(std::string_view topic, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsSubscribtion* sub, AsyncNatsOwnedString /*err*/, void* ctx)
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

namespace detail
{
class AsyncNatsConnectError
{
public:
  AsyncNatsConnectError() = delete;

  AsyncNatsConnectError(::AsyncNatsConnectError* e)
      : e_(e)
  {
    assert(e != nullptr);
  }

  AsyncNatsConnectError(const AsyncNatsConnectError&) = delete;
  AsyncNatsConnectError(AsyncNatsConnectError&& o)
  {
    e_ = o.e_;
    o.e_ = nullptr;
  }

  ~AsyncNatsConnectError()
  {
    if (e_)
      async_nats_connection_error_delete(e_);
  }

  AsyncNatsConnectError& operator=(const AsyncNatsConnectError&) = delete;
  AsyncNatsConnectError& operator=(AsyncNatsConnectError&& o)
  {
    if (e_)
      async_nats_connection_error_delete(e_);

    e_ = o.e_;
    o.e_ = nullptr;
    return *this;
  }

  AsyncNatsConnectErrorKind kind() const
  {
    return async_nats_connection_error_kind(e_);
  }

  std::string message() const
  {
    auto text = async_nats_connection_error_describtion(e_);
    std::string msg(text);
    async_nats_owned_string_delete(text);
    return msg;
  }

private:
  ::AsyncNatsConnectError* e_ = nullptr;
};
}  // namespace detail

class ConnectionError : public std::runtime_error
{
public:
  ConnectionError(detail::AsyncNatsConnectError e)
      : std::runtime_error(e.message())
      , kind_(e.kind())
  {
  }

  AsyncNatsConnectErrorKind kind() const
  {
    return kind_;
  }

private:
  AsyncNatsConnectErrorKind kind_;
};

template<class CompletionToken>
auto connect(const TokioRuntime& rt, const ConnectionOptions& options, CompletionToken&& token)
{
  auto init = [&](auto token)
  {
    using CH = std::decay_t<decltype(token)>;

    static auto f = [](AsyncNatsConnection* conn, AsyncNatsConnectError* e, void* ctx)
    {
      auto c = static_cast<CH*>(ctx);
      if (!conn) {
        (*c)(std::make_exception_ptr(ConnectionError(detail::AsyncNatsConnectError(e))),
             Connection(conn));
      } else {
        (*c)(nullptr, Connection(conn));
      }

      delete c;
    };

    auto ctx = new CH(std::move(token));
    ::AsyncNatsConnectCallback cb {f, ctx};
    async_nats_connection_connect(rt.get_raw(), options.get_raw(), cb);
  };

  return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Connection)>(init,
                                                                                            token);
}

}  // namespace async_nats
