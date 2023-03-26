#ifndef async_nats_CAPI_H
#define async_nats_CAPI_H

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
/// ---------- Common structs ----------
struct AsyncNatsSlice
{
  const uint8_t* data;
  uint64_t size;
};

struct AsyncNatsOwnedString;
void async_nats_owned_string_delete(AsyncNatsOwnedString*);
AsyncNatsSlice async_nats_owned_string_data(const AsyncNatsOwnedString*);

struct AsyncNatsOptionalI32 {
  bool has_value;
  int32_t value;
};

/// ---------- Tokio runtime ----------
/// ---- Runtime Config -----
struct AsyncNatsTokioRuntimeCfg;
AsyncNatsTokioRuntimeCfg* async_nats_tokio_runtime_config_new();
void async_nats_tokio_runtime_config_delete(AsyncNatsTokioRuntimeCfg* cfg);
void async_nats_tokio_runtime_config_thread_name(AsyncNatsTokioRuntimeCfg* cfg, const char* tname);
void async_nats_tokio_runtime_config_thread_count(AsyncNatsTokioRuntimeCfg* cfg, uint32_t count);
/// ---- Runtime -----
struct AsyncNatsTokioRuntime;
AsyncNatsTokioRuntime* async_nats_tokio_runtime_new(const AsyncNatsTokioRuntimeCfg* cfg);
void async_nats_tokio_runtime_delete(AsyncNatsTokioRuntime* runtime);

/// ---------- Nats Connection ----------
/// ---- Nats Connection Config ----
struct AsyncNatsConnectionConfig;
AsyncNatsConnectionConfig* async_nats_runtime_config_new();
void async_nats_runtime_config_free(AsyncNatsConnectionConfig* cfg);
void async_nats_runtime_config_set_endpoint(AsyncNatsConnectionConfig* cfg, const char* data);

/// ---- Connection ----
struct AsyncNatsIoError;
AsyncNatsOptionalI32 async_nats_io_error_system_code(const AsyncNatsIoError*);
AsyncNatsOwnedString* async_nats_io_error_description(const AsyncNatsIoError*);
struct AsyncNatsConnection;
AsyncNatsConnection* async_nats_connection_new(const AsyncNatsConnectionConfig* cfg);
AsyncNatsConnection* async_nats_connection_clone(const AsyncNatsConnection* conn);
void async_nats_connection_delete(AsyncNatsConnection* conn);
AsyncNatsIoError* async_nats_connection_connect(AsyncNatsConnection* conn);

// TODO connection

/// ---- NatsMessage ----
struct AsyncNatsMessage;
void async_nats_message_delete(AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_topic(const AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_data(const AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_reply_to(const AsyncNatsMessage* msg);

/// ---- Subscription ----
struct AsyncNatsSubscription;
typedef void (*RecieveCallbackFunction)(AsyncNatsMessage* msg, void*);
struct AsyncNatsRecieveCallback
{
  RecieveCallbackFunction f;  ///< @brief callback function
  const void* closure;  ///< @brief data to be passed back
};
AsyncNatsSubscription* async_nats_subscribtion_create(const AsyncNatsRuntime* rt,
                                                      const char* topic);
void async_nats_subscribtion_delete(AsyncNatsSubscription* chan);
bool async_nats_subscribtion_is_open(const AsyncNatsSubscription* chan);
// AsyncNatsMessage* async_nats_subscribtion_try_recv(AsyncNatsSubscription* chan);
AsyncNatsMessage* async_nats_subscribtion_recv(AsyncNatsSubscription* chan);
void async_nats_subscribtion_recv_async(AsyncNatsSubscription* chan, AsyncNatsRecieveCallback cb);
// void async_nats_subscribtion_bench(Subscription* chan, uint64_t count);

/// ---- JetStream ----

EXTERN_C_END

#endif  // async_nats_CAPI_H
