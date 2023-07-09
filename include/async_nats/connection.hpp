#pragma once

#include <functional>
#include <string>
#include <string_view>

#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

#include <async_nats/detail/helpers.hpp>
#include <async_nats/errors.hpp>
#include <async_nats/owned_string.h>
#include <async_nats/request.hpp>
#include <async_nats/subscribtion.hpp>
#include <async_nats/tokio_runtime.hpp>

namespace async_nats
{
class ConnectionOptions
{
public:
  ConnectionOptions() noexcept { options_ = async_nats_connection_config_new(); }

  ConnectionOptions(const ConnectionOptions&) noexcept = delete;
  ConnectionOptions(ConnectionOptions&& o) noexcept
  {
    options_ = o.options_;
    o.options_ = nullptr;
  }

  ~ConnectionOptions() noexcept
  {
    if (options_ != nullptr) {
      async_nats_connection_config_delete(options_);
    }
  }

  ConnectionOptions& operator=(const ConnectionOptions&) noexcept = delete;
  ConnectionOptions& operator=(ConnectionOptions&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (options_ != nullptr) {
      async_nats_connection_config_delete(options_);
    }

    options_ = o.options_;
    o.options_ = nullptr;
    return *this;
  }

  ConnectionOptions& name(const std::string& name) noexcept
  {
    async_nats_connection_config_name(options_, name.c_str());
    return *this;
  }

  ConnectionOptions& address(const std::string& addr) noexcept
  {
    async_nats_connection_config_addr(options_, addr.c_str());
    return *this;
  }

  AsyncNatsConnetionParams* get_raw() noexcept { return options_; }

  const AsyncNatsConnetionParams* get_raw() const noexcept { return options_; }

private:
  AsyncNatsConnetionParams* options_;
};

class ConnectionError : public Exception
{
public:
  ConnectionError(::AsyncNatsConnectError* e) noexcept
      : e_(e)
  {
  }

  ConnectionError(const ConnectionError& o) { e_ = async_nats_connection_error_clone(o.e_); }

  ConnectionError(ConnectionError&& o) noexcept
  {
    e_ = o.e_;
    str_ = std::move(o.str_);
    o.e_ = nullptr;
  }

  ~ConnectionError() noexcept override
  {
    if (e_ != nullptr) {
      async_nats_connection_error_delete(e_);
    }
  }

  ConnectionError& operator=(const ConnectionError& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (e_ != nullptr) {
      async_nats_connection_error_delete(e_);
    }

    e_ = async_nats_connection_error_clone(o.e_);
    return *this;
  }

  ConnectionError& operator=(ConnectionError&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (e_ != nullptr) {
      async_nats_connection_error_delete(e_);
    }

    e_ = o.e_;
    str_ = std::move(o.str_);
    o.e_ = nullptr;
    return *this;
  }

  AsyncNatsConnectErrorKind kind() const noexcept { return async_nats_connection_error_kind(e_); }

  // exception interface
  const char* what() const noexcept override
  {
    if (str_) {
      return str_.value();
    }

    assert(e_ != nullptr && "There is no exception to get text from");
    str_ = OwnedString(async_nats_connection_error_describtion(e_));
    return str_.value();
  }

private:
  ::AsyncNatsConnectError* e_;
  /**
   * @brief str_ contains an arror text
   *
   * this varriable is mutable to save an allocation when text is not requested by the user
   */
  mutable std::optional<OwnedString> str_;
};

class RequestError : public Exception
{
public:
  RequestError(::AsyncNatsRequestError* e) noexcept
      : e_(e)
  {
  }

  RequestError(const RequestError& o)
      : e_(async_nats_request_error_clone(o.e_))
  {
  }

  RequestError(RequestError&& o) noexcept
      : e_(o.e_)
      , str_(std::move(o.str_))
  {
    o.e_ = nullptr;
  }

  RequestError& operator=(const RequestError& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (e_ != nullptr) {
      async_nats_request_error_delete(e_);
    }

    e_ = async_nats_request_error_clone(o.e_);
    return *this;
  }

  RequestError& operator=(RequestError&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (e_ != nullptr) {
      async_nats_request_error_delete(e_);
    }

    e_ = o.e_;
    str_ = std::move(o.str_);
    o.e_ = nullptr;
    return *this;
  }

  ~RequestError() noexcept override
  {
    if (e_ != nullptr) {
      async_nats_request_error_delete(e_);
    }
  }

  AsyncNatsRequestErrorKind kind() const noexcept { return async_nats_request_error_kind(e_); }

  // exception interface
  const char* what() const noexcept override
  {
    if (str_) {
      return str_.value();
    }

    assert(e_ != nullptr && "There is no exception to get text from");
    str_ = OwnedString(async_nats_request_error_describtion(e_));
    return str_.value();
  }

private:
  ::AsyncNatsRequestError* e_;
  /**
   * @brief str_ contains an arror text
   *
   * this varriable is mutable to save an allocation when text is not requested by the user
   */
  mutable std::optional<OwnedString> str_;
};

/**
 * @brief The Connection class is used to access nats server
 *
 * @threadsafe This class is thread safe
 */
class Connection
{
public:
  Connection() noexcept = default;

  Connection(AsyncNatsConnection* conn) noexcept
      : conn_(conn)
  {
  }

  Connection(const Connection& o) noexcept { conn_ = async_nats_connection_clone(o.get_raw()); }

  Connection(Connection&& o) noexcept
  {
    conn_ = o.conn_;
    o.conn_ = nullptr;
  }

  ~Connection() noexcept
  {
    if (conn_ != nullptr) {
      async_nats_connection_delete(conn_);
    }
  }

  Connection& operator=(const Connection& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (conn_ != nullptr) {
      async_nats_connection_delete(conn_);
    }
    conn_ = async_nats_connection_clone(o.get_raw());

    return *this;
  }

  Connection& operator=(Connection&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (conn_ != nullptr) {
      async_nats_connection_delete(conn_);
    }
    conn_ = o.conn_;
    o.conn_ = nullptr;

    return *this;
  }

  operator bool() const noexcept { return conn_ != nullptr; }

  AsyncNatsConnection* get_raw() noexcept { return conn_; }

  const AsyncNatsConnection* get_raw() const noexcept { return conn_; }

  /**
   * @brief new_mailbox function generates new random string that can be used for replies
   */
  OwnedString new_mailbox() const noexcept
  {
    return OwnedString(async_nats_connection_mailbox(conn_));
  }

  /**
   * @brief publish
   * @param subject
   * @param data
   * @param token
   */
  template<class CompletionToken>
  auto publish(std::string_view subject,
               boost::asio::const_buffer data,
               CompletionToken&& completion_token)
  {
    auto init = [this](auto token, auto i_subject, auto i_data)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](void* ctx)
      {
        auto* c = static_cast<CH*>(ctx);
        (*c)();
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsPublishCallback cb {f, ctx};
      async_nats_connection_publish_async(get_raw(),
                                          AsyncNatsSlice {i_subject.data(), i_subject.size()},
                                          AsyncNatsBorrowedMessage {i_data.data(), i_data.size()},
                                          cb);
    };

    return boost::asio::async_initiate<CompletionToken, void()>(
        init, completion_token, subject, data);
  }

  /**
   * @brief publish - publish a new message with reply address
   * @param subject
   * @param reply_to - a topic for the reply message
   * @param data
   * @param token
   */
  template<class CompletionToken>
  auto publish(std::string_view subject,
               std::string_view reply_to,
               boost::asio::const_buffer data,
               CompletionToken&& completion_token)
  {
    auto init = [this](auto token, auto i_subject, auto i_reply_to, auto i_data)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](void* ctx)
      {
        auto* c = static_cast<CH*>(ctx);
        (*c)();
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsPublishCallback cb {f, ctx};
      async_nats_connection_publish_with_reply_async(
          get_raw(),
          AsyncNatsSlice {i_subject.data(), i_subject.size()},
          AsyncNatsSlice {i_reply_to.data(), i_reply_to.size()},
          AsyncNatsBorrowedMessage {i_data.data(), i_data.size()},
          cb);
    };

    return boost::asio::async_initiate<CompletionToken, void()>(
        init, completion_token, subject, reply_to, data);
  }

  /// @todo TODO: publish with headers and reply target

  template<class CompletionToken>
  auto subcribe(AsyncNatsAsyncString subject, CompletionToken&& completion_token)
  {
    auto init = [this](auto token, AsyncNatsAsyncString i_subject)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsSubscribtion* sub, AsyncNatsOwnedString /*err*/, void* ctx)
      {
        /// @todo TODO: process error
        auto* c = static_cast<CH*>(ctx);
        (*c)(Subscribtion(sub));
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsSubscribeCallback cb {f, ctx};
      async_nats_connection_subscribe_async(get_raw(), i_subject, cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Subscribtion)>(
        init, completion_token, subject);
  }

  template<class CompletionToken>
  auto request(AsyncNatsAsyncString subject,
               boost::asio::const_buffer data,
               CompletionToken&& completion_token)
  {
    auto init = [this](auto token, auto i_subject, auto i_data)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsMessage* msg, AsyncNatsRequestError* e, void* ctx)
      {
        auto* c = static_cast<CH*>(ctx);
        if (!msg) {
          (*c)(std::make_exception_ptr(RequestError(e)), Message());
        } else {
          (*c)(nullptr, Message(msg));
        }

        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsRequestCallback cb {f, ctx};
      async_nats_connection_request_async(
          conn_, i_subject, AsyncNatsBorrowedMessage {i_data.data(), i_data.size()}, cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Message)>(
        init, completion_token, subject, data);
  }

  template<class CompletionToken>
  auto request(AsyncNatsAsyncString subject,
               RequestBuilder&& req,
               CompletionToken&& completion_token)
  {
    auto init = [this](auto token, auto i_subject, RequestBuilder&& req_builder)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsMessage* msg, AsyncNatsRequestError* e, void* ctx)
      {
        auto* c = static_cast<CH*>(ctx);
        if (!msg) {
          (*c)(std::make_exception_ptr(RequestError(e)), Message());
        } else {
          (*c)(nullptr, Message(msg));
        }
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsRequestCallback cb {f, ctx};
      async_nats_connection_send_request_async(conn_, i_subject, req_builder.release(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Message)>(
        init, completion_token, subject, std::move(req));
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
 *
 * @note options must live untill completion untill the very end of the 
 * asynchronous operation.
 */
template<class CompletionToken>
auto connect(const TokioRuntime& rt,
             const ConnectionOptions& options,
             CompletionToken&& completion_token)
{
  auto init = [](auto token,
                 std::reference_wrapper<const TokioRuntime> i_rt,
                 std::reference_wrapper<const ConnectionOptions> i_options)
  {
    using CH = std::decay_t<decltype(token)>;

    static auto f = [](AsyncNatsConnection* conn, AsyncNatsConnectError* e, void* ctx)
    {
      auto* c = static_cast<CH*>(ctx);
      if (!conn) {
        (*c)(std::make_exception_ptr(ConnectionError(e)), Connection(conn));
      } else {
        (*c)(nullptr, Connection(conn));
      }

      detail::deallocate_ctx(c);
    };

    auto ctx = detail::allocate_ctx(std::move(token));
    const ::AsyncNatsConnectCallback cb {f, ctx};
    async_nats_connection_connect(i_rt.get().get_raw(), i_options.get().get_raw(), cb);
  };

  return boost::asio::async_initiate<CompletionToken, void(std::exception_ptr, Connection)>(
      init, completion_token, std::cref(rt), std::cref(options));
}

}  // namespace async_nats
