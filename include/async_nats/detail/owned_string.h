#pragma once

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
  OwnedString(char* s)
      : s_(s)
  {
  }

  OwnedString(const OwnedString&) = delete;

  OwnedString(OwnedString&& o)
  {
    s_ = o.s_;
    o.s_ = nullptr;
  }

  ~OwnedString()
  {
    if (s_)
      async_nats_owned_string_delete(s_);
  }

  OwnedString& operator=(const OwnedString&) = delete;

  OwnedString& operator=(OwnedString&& o)
  {
    if (this == &o)
      return *this;

    if (s_)
      async_nats_owned_string_delete(s_);

    s_ = o.s_;
    o.s_ = nullptr;
    return *this;
  }

  operator const char*() const
  {
    return s_;
  }

private:
  char* s_ = nullptr;
};

}  // namespace async_nats::detail
