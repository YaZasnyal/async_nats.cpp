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
  SubscribtionCancellationToken() noexcept = default;

  explicit SubscribtionCancellationToken(AsyncNatsSubscribtionCancellationToken* token) noexcept
      : token_(token)
  {
  }

  SubscribtionCancellationToken(const SubscribtionCancellationToken& o) noexcept
  {
    token_ = async_nats_subscribtion_cancellation_token_clone(o.token_);
  }

  SubscribtionCancellationToken(SubscribtionCancellationToken&& o) noexcept
  {
    token_ = o.token_;
    o.token_ = nullptr;
  }

  ~SubscribtionCancellationToken() noexcept
  {
    if (token_ != nullptr) {
      async_nats_subscribtion_cancellation_token_delete(token_);
    }
  }

  SubscribtionCancellationToken& operator=(const SubscribtionCancellationToken& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (token_ != nullptr) {
      async_nats_subscribtion_cancellation_token_delete(token_);
    }

    token_ = async_nats_subscribtion_cancellation_token_clone(o.token_);

    return *this;
  }

  SubscribtionCancellationToken& operator=(SubscribtionCancellationToken&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (token_ != nullptr) {
      async_nats_subscribtion_cancellation_token_delete(token_);
    }

    token_ = o.token_;
    o.token_ = nullptr;

    return *this;
  }

  /**
   * @brief cancel notifies Subscribtion that it should stop receiving new messages from the
   * server.
   */
  void cancel() const noexcept
  {
    if (token_ != nullptr) {
      async_nats_subscribtion_cancellation_token_cancel(token_);
    }
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
  Subscribtion() noexcept = default;

  explicit Subscribtion(AsyncNatsSubscribtion* sub) noexcept
      : sub_(sub)
  {
  }

  Subscribtion(const Subscribtion&) noexcept = delete;
  Subscribtion(Subscribtion&& o) noexcept
  {
    sub_ = o.sub_;
    o.sub_ = nullptr;
  }

  ~Subscribtion() noexcept
  {
    if (sub_ != nullptr) {
      async_nats_subscribtion_delete(sub_);
    }
  }

  Subscribtion& operator=(const Subscribtion&) noexcept = delete;
  Subscribtion& operator=(Subscribtion&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (sub_ != nullptr) {
      async_nats_subscribtion_delete(sub_);
    }
    sub_ = o.sub_;
    o.sub_ = nullptr;

    return *this;
  }

  operator bool() const noexcept { return sub_ != nullptr; }

  AsyncNatsSubscribtion* get_raw() noexcept { return sub_; }

  AsyncNatsSubscribtion* release_raw() noexcept
  {
    auto* result = sub_;
    sub_ = nullptr;
    return result;
  }

  /**
   * @brief get_cancellation_token returns special object that allows to cancel subscribtion in a
   * thread-safe manner.
   */
  [[nodiscard]] SubscribtionCancellationToken get_cancellation_token() noexcept
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
        auto* c = static_cast<CH*>(ctx);
        (*c)(Message(msg));
        detail::deallocate_ctx(c);
      };

      auto ctx = detail::allocate_ctx(std::move(token));
      const ::AsyncNatsReceiveCallback cb {f, ctx};
      async_nats_subscribtion_receive_async(get_raw(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Message)>(init, token);
  }

private:
  AsyncNatsSubscribtion* sub_ = nullptr;
};

}  // namespace async_nats
