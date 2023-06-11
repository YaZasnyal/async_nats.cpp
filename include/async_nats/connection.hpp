#pragma once

#include <string>
#include <string_view>

#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include <async_nats/detail/owned_string.h>
#include <async_nats/request.hpp>
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

class ConnectionError : public std::runtime_error
{
public:
  ConnectionError(::AsyncNatsConnectError* e)
      : std::runtime_error(detail::OwnedString(async_nats_connection_error_describtion(e)))
      , kind_(async_nats_connection_error_kind(e))
  {
    async_nats_connection_error_delete(e);
  }

  AsyncNatsConnectErrorKind kind() const
  {
    return kind_;
  }

private:
  AsyncNatsConnectErrorKind kind_;
};

class RequestError : public std::runtime_error
{
public:
  RequestError(::AsyncNatsRequestError* e)
      : std::runtime_error(detail::OwnedString(async_nats_request_error_describtion(e)))
      , kind_(async_nats_request_error_kind(e))
  {
    async_nats_request_error_delete(e);
  }

  AsyncNatsRequestErrorKind kind() const
  {
    return kind_;
  }

private:
  AsyncNatsRequestErrorKind kind_;
};

/**
 * @brief The Connection class is used to access nats server
 *
 * @threadsafe This class is thread safe
 */
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
      std::string t(topic.data(), topic.size());
      async_nats_connection_publish_async(
          get_raw(),
          AsyncNatsSlice {reinterpret_cast<const uint8_t*>(topic.data()), topic.size()},
          {reinterpret_cast<const char*>(data.data()), data.size()},
          cb);
    };

    return boost::asio::async_initiate<CompletionToken, void()>(init, token);
  }

  template<class CompletionToken>
  auto subcribe(AsyncNatsAsyncString topic, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsSubscribtion* sub, AsyncNatsOwnedString /*err*/, void* ctx)
      {
        /// @todo TODO: process error
        auto c = static_cast<CH*>(ctx);
        (*c)(Subscribtion(sub));
        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsSubscribeCallback cb {f, ctx};
      async_nats_connection_subscribe_async(get_raw(), topic, cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Subscribtion)>(init, token);
  }

  template<class CompletionToken>
  auto request(AsyncNatsAsyncString topic, boost::asio::const_buffer data, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsMessage* msg, AsyncNatsRequestError* e, void* ctx)
      {
        auto c = static_cast<CH*>(ctx);
        if (!msg) {
          (*c)(std::make_exception_ptr(RequestError(e)), Message());
        } else {
          (*c)(nullptr, Message(msg));
        }

        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsRequestCallback cb {f, ctx};
      async_nats_connection_request_async(
          conn_, topic, {reinterpret_cast<const char*>(data.data()), data.size()}, cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Message)>(init,
                                                                                           token);
  }

  template<class CompletionToken>
  auto request(AsyncNatsAsyncString topic, Request&& req, CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsMessage* msg, AsyncNatsRequestError* e, void* ctx)
      {
        auto c = static_cast<CH*>(ctx);
        if (!msg) {
          (*c)(std::make_exception_ptr(RequestError(e)), Message());
        } else {
          (*c)(nullptr, Message(msg));
        }

        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsRequestCallback cb {f, ctx};
      async_nats_connection_send_request_async(conn_, topic, req.release(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Message)>(init,
                                                                                           token);
  }

private:
  AsyncNatsConnection* conn_ = nullptr;
};

/**
 * @brief connect tries to establish a new connection
 *
 * The result of this operation is either established Connection or ConnectionError in
 * std::exception_ptr.
 *
 * @param rt - Tokio runtime object
 * @param options - connection options
 * @param token - asio completion token
 */
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
        (*c)(std::make_exception_ptr(ConnectionError(e)), Connection(conn));
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
