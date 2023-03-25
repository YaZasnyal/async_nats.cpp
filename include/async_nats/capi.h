#ifndef ASYNC_async_nats_CAPI_H
#define ASYNC_async_nats_CAPI_H

#pragma once

#include <stdint.h>

#ifdef __cplusplus
#  define EXTERN_C extern "C"
#  define EXTERN_C_BEGIN \
    extern "C" \
    {
#  define EXTERN_C_END }
#else
#  define EXTERN_C /* Nothing */
#  define EXTERN_C_BEGIN /* Nothing */
#  define EXTERN_C_END /* Nothing */
#endif

EXTERN_C_BEGIN

/// ---- Tokio runtime ----
// tokio cfg
// tokio runtime

/// ---- cfg ----
struct AsyncNatsRuntimeConfig;
AsyncNatsRuntimeConfig* async_nats_runtime_config_new();
void async_nats_runtime_config_free(AsyncNatsRuntimeConfig* cfg);
void async_nats_runtime_config_set_endpoint(AsyncNatsRuntimeConfig* cfg,
                                            const char* data);
// set threads

/// ---- runtime ----
struct AsyncNatsRuntime;
AsyncNatsRuntime* async_nats_runtime_new(const AsyncNatsRuntimeConfig* cfg);
void async_nats_runtime_free(AsyncNatsRuntime* runtime);
// void async_nats_runtime_connect(NatsRuntime* runtime);

/// ---- connection ----
// TODO connection config
// TODO connection

/// ---- NatsMessage ----
struct AsyncNatsMessage;
struct Slice
{
  const uint8_t* data;
  uint64_t size;
};
void async_nats_message_delete(AsyncNatsMessage* msg);
Slice async_nats_message_topic(const AsyncNatsMessage* msg);
Slice async_nats_message_data(const AsyncNatsMessage* msg);
Slice async_nats_message_reply_to(const AsyncNatsMessage* msg);

/// ---- Subscription ----
struct AsyncNatsSubscription;
typedef void (*RecieveCallbackFunction)(AsyncNatsMessage* msg, void*);
struct AsyncNatsRecieveCallback
{
  RecieveCallbackFunction f;  ///< @brief callback function
  const void* closure;  ///< @brief data to be passed back
};
AsyncNatsSubscription* async_nats_subscribtion_create(
    const AsyncNatsRuntime* rt, const char* topic);
void async_nats_subscribtion_delete(AsyncNatsSubscription* chan);
bool async_nats_subscribtion_is_open(const AsyncNatsSubscription* chan);
AsyncNatsMessage* async_nats_subscribtion_try_recv(AsyncNatsSubscription* chan);
AsyncNatsMessage* async_nats_subscribtion_recv(AsyncNatsSubscription* chan);
void async_nats_subscribtion_recv_async(AsyncNatsSubscription* chan,
                                        AsyncNatsRecieveCallback cb);
// void async_nats_subscribtion_bench(Subscription* chan, uint64_t count);

EXTERN_C_END

#endif  // ASYNC_async_nats_CAPI_H
