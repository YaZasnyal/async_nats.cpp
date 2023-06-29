#pragma once

#include <string_view>

#include "capi.h"

namespace async_nats::detail
{
/**
 * @brief The OwnedString class is used to store C-strings returned from the rust part and free
 * them properly
 */
class OwnedString
{
public:
  OwnedString(char* s) noexcept
      : s_(s)
  {
  }

  OwnedString(const OwnedString&) noexcept = delete;

  OwnedString(OwnedString&& o) noexcept
  {
    s_ = o.s_;
    o.s_ = nullptr;
  }

  ~OwnedString() noexcept
  {
    if (s_)
      async_nats_owned_string_delete(s_);
  }

  OwnedString& operator=(const OwnedString&) noexcept = delete;

  OwnedString& operator=(OwnedString&& o) noexcept
  {
    if (this == &o)
      return *this;

    if (s_)
      async_nats_owned_string_delete(s_);

    s_ = o.s_;
    o.s_ = nullptr;
    return *this;
  }

  operator AsyncNatsAsyncString() const noexcept
  {
    return s_;
  }

  operator std::string_view() const noexcept
  {
    return s_;
  }

private:
  char* s_ = nullptr;
};

}  // namespace async_nats::detail
