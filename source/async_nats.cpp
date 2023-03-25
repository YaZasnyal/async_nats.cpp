#include <string>

#include "async_nats/async_nats.hpp"

#include <fmt/core.h>

exported_class::exported_class()
    : m_name {fmt::format("{}", "async_nats")}
{
}

auto exported_class::name() const -> const char*
{
  return m_name.c_str();
}
