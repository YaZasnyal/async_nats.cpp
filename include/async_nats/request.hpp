#pragma once

#include <chrono>

#include <boost/asio/buffer.hpp>

#include "detail/capi.h"

namespace async_nats
{
/**
 * @brief The Request class is used to construct an RPC request
 */
class RequestBuilder
{
public:
  RequestBuilder() noexcept
      : request_(async_nats_request_new())
  {
  }

  RequestBuilder(const RequestBuilder&) noexcept = delete;

  RequestBuilder(RequestBuilder&& o) noexcept
  {
    request_ = o.request_;
    o.request_ = nullptr;
  }

  ~RequestBuilder() noexcept
  {
    if (request_ != nullptr) {
      async_nats_request_delete(request_);
    }
  }

  RequestBuilder& operator=(const RequestBuilder&) noexcept = delete;

  RequestBuilder& operator=(RequestBuilder&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (request_ != nullptr) {
      async_nats_request_delete(request_);
    }

    request_ = o.request_;
    o.request_ = nullptr;

    return *this;
  }

  RequestBuilder& inbox(AsyncNatsBorrowedString inbox) noexcept
  {
    async_nats_request_inbox(request_, inbox);
    return *this;
  }

  RequestBuilder& data(boost::asio::const_buffer data) noexcept
  {
    async_nats_request_message(request_,
                               {reinterpret_cast<const char*>(data.data()), data.size()});
    return *this;
  }

  // @todo TODO: Headers

  RequestBuilder& timeout(std::chrono::steady_clock::duration duration) noexcept
  {
    async_nats_request_timeout(
        request_,
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
    return *this;
  }

  /**
   * @brief release returns control over underlying struct
   */
  AsyncNatsRequest* release() noexcept
  {
    auto* result = request_;
    request_ = nullptr;
    return result;
  }

private:
  AsyncNatsRequest* request_ = nullptr;
};

}  // namespace async_nats
