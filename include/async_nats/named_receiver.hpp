#pragma once

#include "detail/capi.h"
#include "message.hpp"
#include "subscribtion.hpp"

namespace async_nats
{

class NamedReceiver
{
public:
  NamedReceiver(Subscribtion&& sub, unsigned long long capacity = 1024)
  {
    receiver_ = async_nats_named_receiver_new(sub.release_raw(), capacity);
  }

  NamedReceiver(const NamedReceiver&) = delete;

  NamedReceiver(NamedReceiver&& o)
  {
    receiver_ = o.receiver_;
    o.receiver_ = nullptr;
  }

  ~NamedReceiver()
  {
    if (receiver_)
      async_nats_named_receiver_delete(receiver_);
  }

  NamedReceiver& operator=(const NamedReceiver&) = delete;

  NamedReceiver& operator=(NamedReceiver&& o)
  {
    if(this == &o)
      return *this;

    if (receiver_)
      async_nats_named_receiver_delete(receiver_);

    receiver_ = o.receiver_;
    o.receiver_ = nullptr;
    return *this;
  }

  /**
   * @brief recv blocks current thread untill a message is available
   * @return A new message
   */
  Message recv() const { return Message(async_nats_named_receiver_recv(receiver_)); }

  /**
   * @brief try_recv checks if message is available
   *
   * This function never blocks.
   * @return A new message if avalable
   */
  Message try_recv() const { return Message(async_nats_named_receiver_try_recv(receiver_)); }

private:
  AsyncNatsNamedReceiver* receiver_;
};

}  // namespace async_nats
