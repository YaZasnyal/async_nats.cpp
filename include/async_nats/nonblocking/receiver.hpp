#pragma once

#include <async_nats/detail/capi.h>
#include <async_nats/message.hpp>
#include <async_nats/subscribtion.hpp>

namespace async_nats::nonblocking
{
/**
 * @brief The Receiver class
 *
 * @threadsafe this class is thread safe
 */
class Receiver
{
public:
  Receiver(Subscribtion&& sub, unsigned long long capacity = 1024)
  {
    receiver_ = async_nats_named_receiver_new(sub.release_raw(), capacity);
  }

  Receiver(const Receiver& o)
  {
    receiver_ = async_nats_named_receiver_clone(o.receiver_);
  }

  Receiver(Receiver&& o)
  {
    receiver_ = o.receiver_;
    o.receiver_ = nullptr;
  }

  ~Receiver()
  {
    if (receiver_)
      async_nats_named_receiver_delete(receiver_);
  }

  Receiver& operator=(const Receiver& o)
  {
    if (this == &o)
      return *this;

    if (receiver_)
      async_nats_named_receiver_delete(receiver_);

    receiver_ = async_nats_named_receiver_clone(o.receiver_);
    return *this;
  }

  Receiver& operator=(Receiver&& o)
  {
    if (this == &o)
      return *this;

    if (receiver_)
      async_nats_named_receiver_delete(receiver_);

    receiver_ = o.receiver_;
    o.receiver_ = nullptr;
    return *this;
  }

  /**
   * @brief receive blocks current thread untill a message is available
   *
   * If this methods if called from multiple threads only one thread is going to receive a new
   * message.
   *
   * @return A new message
   */
  Message receive() const
  {
    return Message(async_nats_named_receiver_recv(receiver_));
  }

  /**
   * @brief try_receive checks if message is available
   *
   * This function never blocks.
   *
   * @return A new message if avalable
   */
  Message try_receive() const
  {
    return Message(async_nats_named_receiver_try_recv(receiver_));
  }

private:
  AsyncNatsNamedReceiver* receiver_;
};

}  // namespace async_nats::nonblocking
