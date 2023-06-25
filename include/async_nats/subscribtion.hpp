#pragma once

#include <boost/asio/async_result.hpp>

#include <async_nats/detail/capi.h>
#include <async_nats/detail/helpers.hpp>
#include <async_nats/message.hpp>

namespace async_nats
{
/**
 * @brief The SubscribtionCancellationToken class is used for closing subscribtions
 *
 * When cancel() methos is called the client closes subscribtion asynchronously in the TokioRuntime
 * thread. Cancel returns immediately and messages that are left in the channel are not discarded
 * and should be drained as usual.
 *
 * @threadsafe This class is NOT thread safe but multiple cancellation tokens can be created for
 * a single Subscribtion.
 */
class SubscribtionCancellationToken
{
public:
  SubscribtionCancellationToken(AsyncNatsSubscribtionCancellationToken* token)
      : token_(token)
  {
  }

  SubscribtionCancellationToken(const SubscribtionCancellationToken& o)
  {
    token_ = async_nats_subscribtion_cancellation_token_clone(o.token_);
  }

  SubscribtionCancellationToken(SubscribtionCancellationToken&& o)
  {
    token_ = o.token_;
    o.token_ = nullptr;
  }

  ~SubscribtionCancellationToken()
  {
    if (token_)
      async_nats_subscribtion_cancellation_token_delete(token_);
  }

  SubscribtionCancellationToken& operator=(const SubscribtionCancellationToken& o)
  {
    if (this == &o)
      return *this;

    if (token_)
      async_nats_subscribtion_cancellation_token_delete(token_);

    token_ = async_nats_subscribtion_cancellation_token_clone(o.token_);

    return *this;
  }

  SubscribtionCancellationToken& operator=(SubscribtionCancellationToken&& o)
  {
    if (this == &o)
      return *this;

    if (token_)
      async_nats_subscribtion_cancellation_token_delete(token_);

    token_ = o.token_;
    o.token_ = nullptr;

    return *this;
  }

  /**
   * @brief cancel notifies Subscribtion that it should stop receiving new messages from the
   * server.
   */
  void cancel() const
  {
    async_nats_subscribtion_cancellation_token_cancel(token_);
  }

private:
  AsyncNatsSubscribtionCancellationToken* token_ = nullptr;
};

/**
 * @brief The Subscribtion class
 *
 * @threadsafe This class is NOT thread safe
 */
class Subscribtion
{
public:
  Subscribtion() = default;

  Subscribtion(AsyncNatsSubscribtion* sub)
      : sub_(sub)
  {
  }

  Subscribtion(const Subscribtion&) = delete;
  Subscribtion(Subscribtion&& o)
  {
    sub_ = o.sub_;
    o.sub_ = nullptr;
  }

  ~Subscribtion()
  {
    if (sub_)
      async_nats_subscribtion_delete(sub_);
  }

  Subscribtion& operator=(const Subscribtion&) = delete;
  Subscribtion& operator=(Subscribtion&& o)
  {
    if (this == &o)
      return *this;

    if (sub_)
      async_nats_subscribtion_delete(sub_);
    sub_ = o.sub_;
    o.sub_ = nullptr;

    return *this;
  }

  operator bool() const
  {
    return sub_ != nullptr;
  }

  AsyncNatsSubscribtion* get_raw()
  {
    return sub_;
  }

  AsyncNatsSubscribtion* release_raw()
  {
    auto result = sub_;
    sub_ = nullptr;
    return result;
  }

  /**
   * @brief get_cancellation_token returns special object that allows to cancel subscribtion in a
   * thread-safe manner.
   */
  SubscribtionCancellationToken get_cancellation_token()
  {
    return SubscribtionCancellationToken(async_nats_subscribtion_get_cancellation_token(sub_));
  }

  template<class CompletionToken>
  auto receive(CompletionToken&& token)
  {
    auto init = [&](auto token)
    {
      using CH = std::decay_t<decltype(token)>;

      static auto f = [](AsyncNatsMessage* msg, void* ctx)
      {
        auto c = static_cast<CH*>(ctx);
        (*c)(Message(msg));
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      ::AsyncNatsReceiveCallback cb {f, ctx};
      async_nats_subscribtion_receive_async(get_raw(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Message)>(init, token);
  }

private:
  AsyncNatsSubscribtion* sub_ = nullptr;
};

}  // namespace async_nats
