#pragma once

#include <chrono>

#include <boost/asio/buffer.hpp>

#include "detail/capi.h"

namespace async_nats
{
/**
 * @brief The Request class is used to construct an RPC request
 */
class Request
{
public:
  Request()
      : request_(async_nats_request_new())
  {
  }

  Request(const Request&) = delete;

  Request(Request&& o)
  {
    request_ = o.request_;
    o.request_ = nullptr;
  }

  ~Request()
  {
    if (request_)
      async_nats_request_delete(request_);
  }

  Request& operator=(const Request&) = delete;

  Request& operator=(Request&& o)
  {
    if (this == &o)
      return *this;

    if (request_)
      async_nats_request_delete(request_);

    request_ = o.request_;
    o.request_ = nullptr;

    return *this;
  }

  Request& inbox(AsyncNatsBorrowedString inbox)
  {
    async_nats_request_inbox(request_, inbox);
    return *this;
  }

  Request& data(boost::asio::const_buffer data)
  {
    async_nats_request_message(request_,
                               {reinterpret_cast<const char*>(data.data()), data.size()});
    return *this;
  }

  // @todo TODO: Headers

  Request& timeout(std::chrono::steady_clock::duration duration)
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
  AsyncNatsRequest* release()
  {
    auto result = request_;
    request_ = nullptr;
    return result;
  }

private:
  AsyncNatsRequest* request_ = nullptr;
};

}  // namespace async_nats
