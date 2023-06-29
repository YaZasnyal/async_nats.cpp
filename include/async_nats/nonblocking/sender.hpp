#pragma once

#include <string>
#include <string_view>

#include <boost/asio/buffer.hpp>

#include <async_nats/connection.hpp>
#include <async_nats/detail/capi.h>

namespace async_nats::nonblocking
{

/**
 * @brief The Sender class
 *
 * @threadsafe this class is thread safe
 */
class Sender
{
public:
  Sender(AsyncNatsAsyncString topic, const Connection& conn) noexcept
  {
    sender = async_nats_named_sender_new(topic, conn.get_raw(), 128);
  }

  Sender(const char* topic, const Connection& conn, std::size_t capacity) noexcept
  {
    sender = async_nats_named_sender_new(topic, conn.get_raw(), capacity);
  }

  explicit Sender(const std::string& topic, const Connection& conn) noexcept
  {
    sender = async_nats_named_sender_new(topic.c_str(), conn.get_raw(), 128);
  }

  explicit Sender(const std::string& topic, const Connection& conn, std::size_t capacity) noexcept
  {
    sender = async_nats_named_sender_new(topic.c_str(), conn.get_raw(), capacity);
  }

  Sender(const Sender& o) noexcept
  {
    sender = async_nats_named_sender_clone(o.get_raw());
  }

  Sender(Sender&& o) noexcept
  {
    sender = o.sender;
    o.sender = nullptr;
  }

  ~Sender() noexcept
  {
    if (sender)
      async_nats_named_sender_delete(sender);
  }

  Sender& operator=(const Sender& o) noexcept
  {
    if (this == &o)
      return *this;

    if (sender)
      async_nats_named_sender_delete(sender);

    sender = async_nats_named_sender_clone(o.get_raw());
    return *this;
  }

  Sender& operator=(Sender&& o) noexcept
  {
    if (this == &o)
      return *this;

    if (sender)
      async_nats_named_sender_delete(sender);

    sender = o.sender;
    o.sender = nullptr;

    return *this;
  }

  AsyncNatsNamedSender* get_raw() noexcept
  {
    return sender;
  }

  const AsyncNatsNamedSender* get_raw() const noexcept
  {
    return sender;
  }

  /**
   * @brief try_send - pushes data to the send queue if there is space available
   * @return true if message has been enqueued
   */
  bool try_send(boost::asio::const_buffer data) const noexcept
  {
    return async_nats_named_sender_try_send(
        sender, nullptr, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

  bool try_send(const char* topic, boost::asio::const_buffer data) const noexcept
  {
    return async_nats_named_sender_try_send(
        sender, topic, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

  /**
   * @brief try_send - pushes data to the send queue even if there is no space available
   * @return true if message has been enqueued
   *
   * @note this may cause running out of memory
   */
  void send(boost::asio::const_buffer data) const noexcept
  {
    async_nats_named_sender_send(
        sender, nullptr, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

  void send(const char* topic, boost::asio::const_buffer data) const noexcept
  {
    async_nats_named_sender_send(
        sender, topic, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

private:
  AsyncNatsNamedSender* sender;
};

}  // namespace async_nats
