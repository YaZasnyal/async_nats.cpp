#pragma once

#include <string>

#include "async_nats/detail/capi.h"

namespace async_nats {

class TokioRuntimeConfig {
public:
  TokioRuntimeConfig()
  {
    cfg_ = async_nats_tokio_runtime_config_new();
  }

  ~TokioRuntimeConfig()
  {
    async_nats_tokio_runtime_config_delete(cfg_);
  }

  TokioRuntimeConfig& thread_name(const std::string& tname) {
    async_nats_tokio_runtime_config_thread_name(cfg_, tname.c_str());
    return *this;
  }

  TokioRuntimeConfig& thread_count(uint32_t tcount) {
    async_nats_tokio_runtime_config_thread_count(cfg_, tcount);
    return *this;
  }

  AsyncNatsTokioRuntimeCfg* get_raw() {
    return cfg_;
  }

  const AsyncNatsTokioRuntimeCfg* get_raw() const {
    return cfg_;
  }

private:
  AsyncNatsTokioRuntimeCfg* cfg_;
};

class TokioRuntime
{
public:
  TokioRuntime()
  {
    TokioRuntimeConfig cfg;
    rt_ = async_nats_tokio_runtime_new(cfg.get_raw());
  }

  TokioRuntime(const TokioRuntimeConfig& cfg)
  {
    rt_ = async_nats_tokio_runtime_new(cfg.get_raw());
  }

  ~TokioRuntime()
  {
    async_nats_tokio_runtime_delete(rt_);
  }

  AsyncNatsTokioRuntime* get_raw() {
    return rt_;
  }

  const AsyncNatsTokioRuntime* get_raw() const {
    return rt_;
  }

private:
  AsyncNatsTokioRuntime* rt_;
};

}
