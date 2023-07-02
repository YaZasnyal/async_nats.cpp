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
  static constexpr std::size_t default_capacity = 128;

  Sender(AsyncNatsAsyncString topic, const Connection& conn) noexcept
      : sender_(async_nats_named_sender_new(topic, conn.get_raw(), default_capacity))
  {
  }

  Sender(const char* topic, const Connection& conn, std::size_t capacity) noexcept
      : sender_(async_nats_named_sender_new(topic, conn.get_raw(), capacity))
  {
  }

  explicit Sender(const std::string& topic, const Connection& conn) noexcept
      : sender_(async_nats_named_sender_new(topic.c_str(), conn.get_raw(), default_capacity))
  {
  }

  explicit Sender(const std::string& topic, const Connection& conn, std::size_t capacity) noexcept
      : sender_(async_nats_named_sender_new(topic.c_str(), conn.get_raw(), capacity))
  {
  }

  Sender(const Sender& o) noexcept
      : sender_(async_nats_named_sender_clone(o.get_raw()))
  {
  }

  Sender(Sender&& o) noexcept
      : sender_(o.sender_)
  {
    o.sender_ = nullptr;
  }

  ~Sender() noexcept
  {
    if (sender_ != nullptr) {
      async_nats_named_sender_delete(sender_);
    }
  }

  Sender& operator=(const Sender& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (sender_ != nullptr) {
      async_nats_named_sender_delete(sender_);
    }

    sender_ = async_nats_named_sender_clone(o.get_raw());
    return *this;
  }

  Sender& operator=(Sender&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (sender_ != nullptr) {
      async_nats_named_sender_delete(sender_);
    }

    sender_ = o.sender_;
    o.sender_ = nullptr;

    return *this;
  }

  AsyncNatsNamedSender* get_raw() noexcept { return sender_; }

  const AsyncNatsNamedSender* get_raw() const noexcept { return sender_; }

  /**
   * @brief try_send - pushes data to the send queue if there is space available
   * @return true if message has been enqueued
   */
  bool try_send(boost::asio::const_buffer data) const noexcept
  {
    return async_nats_named_sender_try_send(
        sender_, nullptr, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

  bool try_send(const char* topic, boost::asio::const_buffer data) const noexcept
  {
    return async_nats_named_sender_try_send(
        sender_, topic, {reinterpret_cast<const char*>(data.data()), data.size()});
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
        sender_, nullptr, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

  void send(const char* topic, boost::asio::const_buffer data) const noexcept
  {
    async_nats_named_sender_send(
        sender_, topic, {reinterpret_cast<const char*>(data.data()), data.size()});
  }

private:
  AsyncNatsNamedSender* sender_;
};

}  // namespace async_nats::nonblocking
