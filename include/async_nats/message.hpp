#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <async_nats/detail/capi.h>
#include <async_nats/owned_string.h>

namespace async_nats
{
class Message;
class HeadersView;

class HeaderVectorView
{
public:
  HeaderVectorView(const HeaderVectorView& o) noexcept = delete;
  HeaderVectorView(HeaderVectorView&& o) noexcept
  {
    owns_ = o.owns_;
    it_ = o.it_;
    size_ = o.size_;

    o.it_ = nullptr;
  }

  ~HeaderVectorView() noexcept
  {
    if (owns_ && (it_ != nullptr)) {
      async_nats_message_header_iterator_free(it_);
    }
  }

  HeaderVectorView& operator=(const HeaderVectorView&) noexcept = delete;
  HeaderVectorView& operator=(HeaderVectorView&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (owns_ && (it_ != nullptr)) {
      async_nats_message_header_iterator_free(it_);
    }

    owns_ = o.owns_;
    it_ = o.it_;
    size_ = o.size_;

    return *this;
  }

  size_t size() const noexcept
  {
    return size_;
  }

  std::string_view at(size_t index) const
  {
    if (index >= size_) {
      throw std::out_of_range("HeaderVectorView index is out of range");
    }

    auto slice = async_nats_message_header_iterator_value_at(it_, index);
    return std::string_view(static_cast<const char*>(slice.data), slice.size);
  }

  class Iterator
  {
  public:
    void operator++() noexcept
    {
      if (pos_ < size_) {
        ++pos_;
      }
    }

    void operator+(size_t size) noexcept
    {
      if (pos_ + size < size_) {
        pos_ += size;
      }
    }

    void operator--() noexcept
    {
      if (pos_ != 0) {
        --pos_;
      }
    }

    void operator-(size_t size) noexcept
    {
      if (pos_ <= size) {
        pos_ -= size;
      }
    }

    bool operator==(const Iterator& o) const noexcept
    {
      return pos_ == o.pos_;
    }

    bool operator!=(const Iterator& o) const noexcept
    {
      return !(pos_ == o.pos_);
    }

    std::string_view operator*() const
    {
      if (pos_ >= size_) {
        throw std::out_of_range("Iterator is out of range");
      }

      auto slice = async_nats_message_header_iterator_value_at(it_, pos_);
      return std::string_view(static_cast<const char*>(slice.data), slice.size);
    }

  private:
    friend class HeaderVectorView;
    Iterator(AsyncNatsHeaderIterator* it, size_t pos, size_t size) noexcept
        : it_(it)
        , pos_(pos)
        , size_(size)
    {
    }

    AsyncNatsHeaderIterator* it_;
    size_t pos_ = 0;
    size_t size_ = 0;
  };

  Iterator begin() const noexcept
  {
    return Iterator(it_, 0, size_);
  }

  Iterator end() const noexcept
  {
    return Iterator(it_, size_, size_);
  }

private:
  friend class HeadersView;
  explicit HeaderVectorView(AsyncNatsHeaderIterator* it, bool own = false) noexcept
      : owns_(own)
      , it_(it)
  {
    size_ = async_nats_message_header_iterator_value_count(it_);
  }

  bool owns_ = false;
  AsyncNatsHeaderIterator* it_;
  size_t size_ = 0;
};

class HeadersView
{
public:
  class HeaderIterator
  {
  public:
    HeaderIterator(const HeaderIterator&) = delete;

    HeaderIterator(HeaderIterator&& o) noexcept
        : it_(o.it_)
    {
      o.it_ = nullptr;
    }

    ~HeaderIterator() noexcept
    {
      if (it_ != nullptr) {
        async_nats_message_header_iterator_free(it_);
      }
    }

    HeaderIterator& operator=(const HeaderIterator&) = delete;

    HeaderIterator& operator=(HeaderIterator&& o) noexcept
    {
      if (this == &o) {
        return *this;
      }

      if (it_ != nullptr) {
        async_nats_message_header_iterator_free(it_);
      }

      it_ = o.it_;
      o.it_ = nullptr;

      return *this;
    }

    std::pair<std::string_view, HeaderVectorView> operator*() const noexcept
    {
      auto h = async_nats_message_header_iterator_key(it_);
      return std::make_pair(std::string_view(static_cast<const char*>(h.data), h.size),
                            HeaderVectorView(it_));
    }

    /**
     * @brief operator++ moves cursor to the next header
     *
     * @note invalidates both HeaderIterator and assotiated HeaderVectorView
     */
    void operator++() noexcept
    {
      if (!async_nats_message_header_iterator_next(it_)) {
        if (it_ != nullptr) {
          async_nats_message_header_iterator_free(it_);
        }
        it_ = nullptr;
      }
    }

    bool operator==(HeaderIterator& o) const noexcept
    {
      return o.it_ != it_;
    }

    bool operator!=(HeaderIterator& o) const noexcept
    {
      return !(o.it_ == it_);
    }

  private:
    friend class HeadersView;
    HeaderIterator() noexcept = default;
    explicit HeaderIterator(AsyncNatsHeaderIterator* it) noexcept
        : it_(it)
    {
      ++(*this);
    }

    AsyncNatsHeaderIterator* it_ = nullptr;
  };

  HeadersView(const HeadersView&) = delete;

  HeadersView(HeadersView&& o) noexcept
      : message_(o.message_)
  {
    o.message_ = nullptr;
  }

  ~HeadersView() noexcept
  {
    if (message_ != nullptr) {
      async_nats_message_delete(message_);
    }
  }

  HeadersView& operator=(const HeadersView&) = delete;

  HeadersView& operator=(HeadersView&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (message_ != nullptr) {
      async_nats_message_delete(message_);
    }

    message_ = o.message_;
    o.message_ = nullptr;

    return *this;
  }

  /**
   * @brief operator bool allows to theck if there are any headers in the message
   */
  operator bool() const noexcept
  {
    return async_nats_message_has_headers(message_);
  }

  std::optional<HeaderVectorView> get_header(std::string_view header) noexcept
  {
    if (header.empty()) {
      return std::nullopt;
    }

    auto* res =
        async_nats_message_get_header(message_, AsyncNatsSlice {header.data(), header.size()});
    if (res == nullptr) {
      return std::nullopt;
    }

    return HeaderVectorView(res, /*own=*/true);
  }

  /**
   * @brief begin returns iterator to the beginning of the header map
   */
  HeaderIterator begin() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");

    if (!async_nats_message_has_headers(message_)) {
      return HeaderIterator();
    }
    return HeaderIterator(async_nats_message_header_iterator(message_));
  }

  static HeaderIterator end() noexcept
  {
    return HeaderIterator();
  }

  // end

private:
  friend class Message;
  explicit HeadersView(AsyncNatsMessage* message) noexcept
  {
    if (message != nullptr) {
      message_ = async_nats_message_clone(message);
    }
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
  enum StatusCodes : uint16_t
  {
    None = 0,
    IdleHeartbeat = 100,
    Ok = 200,
    NotFound = 404,
    Timeout = 408,
    NoResponders = 503,
    RequestTerminated = 409,
  };

  Message() noexcept = default;

  explicit Message(AsyncNatsMessage* message) noexcept
      : message_(message)
  {
  }

  Message(const Message& o) noexcept
  {
    if (o.message_ == nullptr) {
      return;
    }

    message_ = async_nats_message_clone(o.message_);
  }

  Message(Message&& o) noexcept
  {
    message_ = o.message_;
    o.message_ = nullptr;
  }

  ~Message() noexcept
  {
    if (message_ != nullptr) {
      async_nats_message_delete(message_);
    }
  }

  Message& operator=(const Message& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (message_ != nullptr) {
      async_nats_message_delete(message_);
      message_ = nullptr;
    }

    if (o.message_ == nullptr) {
      return *this;
    }

    message_ = async_nats_message_clone(o.message_);
    return *this;
  }

  Message& operator=(Message&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (message_ != nullptr) {
      async_nats_message_delete(message_);
    }
    message_ = o.message_;
    o.message_ = nullptr;

    return *this;
  }

  operator bool() const noexcept
  {
    return message_ != nullptr;
  }

  std::string_view topic() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_topic(message_);
    return std::string_view(static_cast<const char*>(slice.data), slice.size);
  }

  std::string_view data() const noexcept
  {
    auto slice = async_nats_message_data(message_);
    return std::string_view(static_cast<const char*>(slice.data), slice.size);
  }

  std::optional<std::string_view> reply_to() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_reply_to(message_);
    if (slice.data != nullptr) {
      return std::string_view(static_cast<const char*>(slice.data), slice.size);
    }

    return std::nullopt;
  }

  HeadersView headers() const noexcept
  {
    return HeadersView(message_);
  }

  /**
   * @brief status returns optional status of the message. Used mostly for internal handling
   *
   * Known status fields are listed in StatusCodes enumerator
   */
  uint16_t status() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    return async_nats_message_status(message_);
  }

  std::optional<std::string_view> description() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    auto slice = async_nats_message_description(message_);
    if (slice.data != nullptr) {
      return std::string_view(static_cast<const char*>(slice.data), slice.size);
    }
    return std::nullopt;
  }

  /**
   * @brief length returns length of the message over the wire
   */
  uint64_t length() const noexcept
  {
    return async_nats_message_length(message_);
  }

  OwnedString to_string() const noexcept
  {
    assert(message_ != nullptr && "Message must be checked for null before usage");
    return OwnedString(async_nats_message_to_string(message_));
  }

private:
  AsyncNatsMessage* message_ = nullptr;
};

}  // namespace async_nats
