#pragma once

#include <optional>
#include <string_view>

#include "async_nats/detail/capi.h"

namespace async_nats
{

class Message
{
public:
  Message() = default;

  Message(AsyncNatsMessage* message)
      : message_(message)
  {
  }

  Message(const Message&) = delete;
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

  Message& operator=(const Message& o) = delete;
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

  operator bool() const { return message_ != nullptr; }

  std::string_view Topic() const
  {
    auto slice = async_nats_message_topic(message_);
    return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
  }

  std::string_view Data() const
  {
    auto slice = async_nats_message_data(message_);
    return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
  }

  std::optional<std::string_view> ReplyTo() const
  {
    auto slice = async_nats_message_reply_to(message_);
    if (slice.data) {
      return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
    } else {
      return std::nullopt;
    }
  }

private:
  AsyncNatsMessage* message_ = nullptr;
};

}  // namespace async_nats
