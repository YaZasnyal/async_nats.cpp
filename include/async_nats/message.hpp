#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <string_view>

#include <async_nats/detail/capi.h>
#include <async_nats/detail/owned_string.h>

namespace async_nats
{
/**
 * @brief The Message class stores a single message received from the NATS server
 *
 * This class is cheap to copy and move around.
 *
 * @threadsafe this class is thread safe
 */
class Message
{
public:
  Message() = default;

  Message(AsyncNatsMessage* message)
      : message_(message)
  {
  }

  Message(const Message& o)
  {
    if (!o.message_)
      return;

    message_ = async_nats_message_clone(o.message_);
  }

  Message(Message&& o)
  {
    message_ = o.message_;
    o.message_ = nullptr;
  }

  ~Message()
  {
    if (message_)
      async_nats_message_delete(message_);
  }

  Message& operator=(const Message& o)
  {
    if (this == &o)
      return *this;

    if (message_) {
      async_nats_message_delete(message_);
      message_ = nullptr;
    }

    if (!o.message_)
      return *this;

    message_ = async_nats_message_clone(o.message_);
    return *this;
  }

  Message& operator=(Message&& o)
  {
    if (this == &o)
      return *this;

    if (message_)
      async_nats_message_delete(message_);
    message_ = o.message_;
    o.message_ = nullptr;

    return *this;
  }

  operator bool() const
  {
    return message_ != nullptr;
  }

  std::string_view topic() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_topic(message_);
    return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
  }

  std::string_view data() const
  {
    auto slice = async_nats_message_data(message_);
    return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
  }

  std::optional<std::string_view> reply_to() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_reply_to(message_);
    if (slice.data) {
      return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
    } else {
      return std::nullopt;
    }
  }

  detail::OwnedString to_string() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    return detail::OwnedString(async_nats_message_to_string(message_));
  }

private:
  AsyncNatsMessage* message_ = nullptr;
};

}  // namespace async_nats
