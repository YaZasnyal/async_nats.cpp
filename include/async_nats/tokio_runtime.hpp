#pragma once

#include <string>

#include "async_nats/detail/capi.h"

namespace async_nats
{
class TokioRuntimeConfig
{
public:
  TokioRuntimeConfig() noexcept
      : cfg_(async_nats_tokio_runtime_config_new())
  {
  }

  TokioRuntimeConfig(const TokioRuntimeConfig&) = delete;

  TokioRuntimeConfig(TokioRuntimeConfig&& o) noexcept
      : cfg_(o.cfg_)
  {
    o.cfg_ = nullptr;
  }

  ~TokioRuntimeConfig() noexcept
  {
    if (cfg_ != nullptr) {
      async_nats_tokio_runtime_config_delete(cfg_);
    }
  }

  TokioRuntimeConfig& operator=(const TokioRuntimeConfig&) = delete;

  TokioRuntimeConfig& operator=(TokioRuntimeConfig&& o) noexcept
  {
    if (this == &o) {
      return *this;
    }

    if (cfg_ != nullptr) {
      async_nats_tokio_runtime_config_delete(cfg_);
    }

    cfg_ = o.cfg_;
    o.cfg_ = nullptr;

    return *this;
  }

  TokioRuntimeConfig& thread_name(const std::string& tname) noexcept
  {
    async_nats_tokio_runtime_config_thread_name(cfg_, tname.c_str());
    return *this;
  }

  TokioRuntimeConfig& thread_count(uint32_t tcount) noexcept
  {
    async_nats_tokio_runtime_config_thread_count(cfg_, tcount);
    return *this;
  }

  AsyncNatsTokioRuntimeConfig* get_raw() noexcept { return cfg_; }

  const AsyncNatsTokioRuntimeConfig* get_raw() const noexcept { return cfg_; }

private:
  AsyncNatsTokioRuntimeConfig* cfg_;
};

class TokioRuntime
{
public:
  explicit TokioRuntime(const TokioRuntimeConfig& cfg = TokioRuntimeConfig()) noexcept
  {
    rt_ = async_nats_tokio_runtime_new(cfg.get_raw());
  }

  TokioRuntime(const TokioRuntime&) noexcept = delete;

  TokioRuntime(TokioRuntime&& o) noexcept
      : rt_(o.rt_)
  {
    o.rt_ = nullptr;
  }

  ~TokioRuntime() noexcept
  {
    if (rt_ != nullptr) {
      async_nats_tokio_runtime_delete(rt_);
    }
  }

  TokioRuntime& operator=(const TokioRuntime&) noexcept = delete;

  TokioRuntime& operator=(TokioRuntime&& other) noexcept
  {
    if (this == &other) {
      return *this;
    }

    if (rt_ != nullptr) {
      async_nats_tokio_runtime_delete(rt_);
    }

    rt_ = other.rt_;
    other.rt_ = nullptr;

    return *this;
  }

  operator bool() const noexcept { return rt_ != nullptr; }

  const AsyncNatsTokioRuntime* get_raw() const noexcept { return rt_; }

private:
  AsyncNatsTokioRuntime* rt_;
};

}  // namespace async_nats
