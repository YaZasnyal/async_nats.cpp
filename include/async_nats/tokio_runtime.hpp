#pragma once

#include <string>

#include "async_nats/detail/capi.h"

namespace async_nats
{

class TokioRuntimeConfig
{
public:
  TokioRuntimeConfig() { cfg_ = async_nats_tokio_runtime_config_new(); }

  ~TokioRuntimeConfig() { async_nats_tokio_runtime_config_delete(cfg_); }

  TokioRuntimeConfig& thread_name(const std::string& tname)
  {
    async_nats_tokio_runtime_config_thread_name(cfg_, tname.c_str());
    return *this;
  }

  TokioRuntimeConfig& thread_count(uint32_t tcount)
  {
    async_nats_tokio_runtime_config_thread_count(cfg_, tcount);
    return *this;
  }

  AsyncNatsTokioRuntimeCfg* get_raw() { return cfg_; }

  const AsyncNatsTokioRuntimeCfg* get_raw() const { return cfg_; }

private:
  AsyncNatsTokioRuntimeCfg* cfg_;
};

class TokioRuntime
{
public:
  TokioRuntime(const TokioRuntimeConfig& cfg = TokioRuntimeConfig())
  {
    rt_ = async_nats_tokio_runtime_new(cfg.get_raw());
  }

  TokioRuntime(const TokioRuntime&) = delete;

  TokioRuntime(TokioRuntime&& o)
  {
    rt_ = o.rt_;
    o.rt_ = nullptr;
  }

  ~TokioRuntime()
  {
    if (rt_)
      async_nats_tokio_runtime_delete(rt_);
  }

  TokioRuntime& operator=(const TokioRuntime&) = delete;

  TokioRuntime& operator=(TokioRuntime&& o)
  {
    if (this == &o)
      return *this;

    if (rt_)
      async_nats_tokio_runtime_delete(rt_);

    rt_ = o.rt_;
    o.rt_ = nullptr;

    return *this;
  }

  operator bool() const { return rt_ != nullptr; }

  AsyncNatsTokioRuntime* get_raw() { return rt_; }

  const AsyncNatsTokioRuntime* get_raw() const { return rt_; }

private:
  AsyncNatsTokioRuntime* rt_;
};

}  // namespace async_nats
