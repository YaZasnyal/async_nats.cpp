#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <async_nats/detail/capi.h>
#include <async_nats/detail/owned_string.h>

namespace async_nats
{
class Message;
class HeadersView;

class HeaderVectorView
{
public:
  HeaderVectorView(const HeaderVectorView& o) = delete;
  HeaderVectorView(HeaderVectorView&& o)
  {
    own_ = o.own_;
    it_ = o.it_;
    size_ = o.size_;

    o.it_ = nullptr;
  }

  ~HeaderVectorView()
  {
    if(own_ && it_)
      async_nats_message_header_iterator_free(it_);
  }

  HeaderVectorView& operator=(const HeaderVectorView&) = delete;
  HeaderVectorView& operator==(HeaderVectorView&&) = delete;

  size_t size() const
  {
    return size_;
  }

  std::string_view at(size_t index) const
  {
    if (index >= size_)
      throw std::out_of_range("HeaderVectorView index is out of range");

    auto slice = async_nats_message_header_iterator_value_at(it_, index);
    return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
  }

  class Iterator
  {
  public:
    void operator++()
    {
      if (pos_ < size_)
        ++pos_;
    }

    void operator+(size_t size)
    {
      if (pos_ + size < size_)
        pos_ += size;
    }

    void operator--()
    {
      if (pos_ != 0)
        --pos_;
    }

    void operator-(size_t size)
    {
      if (pos_ <= size)
        pos_ -= size;
    }

    bool operator==(const Iterator& o) const
    {
      return pos_ == o.pos_;
    }

    bool operator!=(const Iterator& o) const
    {
      return !(pos_ == o.pos_);
    }

    std::string_view operator*() const
    {
      if (pos_ >= size_)
        throw std::out_of_range("Iterator is out of range");

      auto slice = async_nats_message_header_iterator_value_at(it_, pos_);
      return std::string_view(reinterpret_cast<const char*>(slice.data), slice.size);
    }

  private:
    friend class HeaderVectorView;
    Iterator(AsyncNatsHeaderIterator* it, size_t pos, size_t size)
        : it_(it)
        , pos_(pos)
        , size_(size)
    {
    }

    AsyncNatsHeaderIterator* it_;
    size_t pos_ = 0;
    size_t size_ = 0;
  };

  Iterator begin() const
  {
    return Iterator(it_, 0, size_);
  }

  Iterator end() const
  {
    return Iterator(it_, size_, size_);
  }

private:
  friend class HeadersView;
  HeaderVectorView(AsyncNatsHeaderIterator* it, bool own = false)
      : own_(own)
      , it_(it)
  {
//    async_nats_message_header_iterator_copy(it_);
    size_ = async_nats_message_header_iterator_value_count(it_);
  }

  bool own_ = false;
  AsyncNatsHeaderIterator* it_;
  size_t size_ = 0;
};

class HeadersView
{
public:
  class HeaderIterator
  {
  public:
    ~HeaderIterator()
    {
      if (it_)
        async_nats_message_header_iterator_free(it_);
    }

    std::pair<std::string_view, HeaderVectorView> operator*() const
    {
      auto h = async_nats_message_header_iterator_key(it_);
      return std::make_pair(std::string_view(reinterpret_cast<const char*>(h.data), h.size),
                            HeaderVectorView(it_));
    }

    /**
     * @brief operator++ moves cursor to the next header
     *
     * @note invalidates both HeaderIterator and assotiated HeaderVectorView
     */
    void operator++()
    {
      if (!async_nats_message_header_iterator_next(it_)) {
        if (it_)
          async_nats_message_header_iterator_free(it_);
        it_ = nullptr;
      }
    }

    bool operator==(HeaderIterator& o) const
    {
      return o.it_ != it_;
    }

    bool operator!=(HeaderIterator& o) const
    {
      return !(o.it_ == it_);
    }

  private:
    friend class HeadersView;
    HeaderIterator() = default;
    HeaderIterator(AsyncNatsHeaderIterator* it)
        : it_(it)
    {
      ++(*this);
    }

    AsyncNatsHeaderIterator* it_ = nullptr;
  };

  ~HeadersView()
  {
    if (message_)
      async_nats_message_delete(message_);
  }

  /**
   * @brief operator bool allows to theck if there are any headers in the message
   */
  operator bool() const
  {
    return async_nats_message_has_headers(message_);
  }

  std::optional<HeaderVectorView> get_header(std::string_view header)
  {
    if (header.empty())
      return std::nullopt;

    auto res = async_nats_message_get_header(
        message_, AsyncNatsSlice {reinterpret_cast<const uint8_t*>(header.data()), header.size()});
    if(res == nullptr)
      return std::nullopt;

    return HeaderVectorView(res, true);
  }

  /**
   * @brief begin returns iterator to the beginning of the header map
   */
  HeaderIterator begin() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");

    if (!async_nats_message_has_headers(message_))
      return HeaderIterator();
    return HeaderIterator(async_nats_message_header_iterator(message_));
  }

  HeaderIterator end() const
  {
    return HeaderIterator();
  }

  // end

private:
  friend class Message;
  HeadersView(AsyncNatsMessage* message)
  {
    if (message)
      message_ = async_nats_message_clone(message);
  }

  AsyncNatsMessage* message_ = nullptr;
};

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

  HeadersView headers() const
  {
    return HeadersView(message_);
  }

  uint16_t status() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    return async_nats_message_status(message_);
  }

  std::optional<std::string_view> description() const
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_description(message_);
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
