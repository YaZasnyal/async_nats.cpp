#pragma once

#include <boost/asio/async_result.hpp>

#include "async_nats/detail/capi.h"
#include "async_nats/message.hpp"

namespace async_nats
{

class Subscribtion
{
public:
  Subscribtion() = default;

  Subscribtion(AsyncNatsSubscription* sub)
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

  operator bool() const { return sub_ != nullptr; }

  AsyncNatsSubscription* get_raw() { return sub_; }

  AsyncNatsSubscription* release_raw()
  {
    auto result = sub_;
    sub_ = nullptr;
    return result;
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
        delete c;
      };

      auto ctx = new CH(std::move(token));
      ::AsyncNatsRecieveCallback cb {f, ctx};
      async_nats_subscribtion_receive_async(get_raw(), cb);
    };

    return boost::asio::async_initiate<CompletionToken, void(Message)>(init, token);
  }

private:
  AsyncNatsSubscription* sub_ = nullptr;
};

}  // namespace async_nats
