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
  unsigned long long size;
};

typedef char* AsyncNatsOwnedString;
void async_nats_owned_string_delete(AsyncNatsOwnedString);

typedef const char* AsyncNatsAsyncString;

struct AsyncNatsOptionalI32
{
  bool has_value;
  int32_t value;
};

typedef AsyncNatsSlice AsyncNatsBorrowedMessage;
/**
 * @brief AsyncNatsAsyncMessage must live long enough to complete an async operation
 */
typedef AsyncNatsSlice AsyncNatsAsyncMessage;

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
typedef int32_t AsyncNatsIoError;
AsyncNatsOptionalI32 async_nats_io_error_system_code(AsyncNatsIoError);
AsyncNatsOwnedString async_nats_io_error_description(AsyncNatsIoError);

struct AsyncNatsConnectionConfig;
AsyncNatsConnectionConfig* async_nats_connection_config_new();
void async_nats_connection_config_delete(AsyncNatsConnectionConfig* cfg);
void async_nats_connection_config_name(AsyncNatsConnectionConfig* cfg, const char* name);
void async_nats_connection_config_addr(AsyncNatsConnectionConfig* cfg, const char* addr);

struct AsyncNatsConnection;
struct AsyncNatsSubscription;
// AsyncNatsConnection* async_nats_connection_new(const AsyncNatsConnectionConfig* cfg);
typedef void (*AsyncNatsConnectCallbackFunction)(AsyncNatsConnection* msg,
                                                 AsyncNatsIoError err,
                                                 void*);
struct AsyncNatsConnectCallback
{
  AsyncNatsConnectCallbackFunction f;  ///< @brief callback function
  void* closure;  ///< @brief data to be passed back
};
void async_nats_connection_connect(const AsyncNatsTokioRuntime* rt,
                                   const AsyncNatsConnectionConfig* cfg,
                                   AsyncNatsConnectCallback cb);
AsyncNatsConnection* async_nats_connection_clone(const AsyncNatsConnection* conn);
void async_nats_connection_delete(AsyncNatsConnection* conn);

typedef void (*AsyncNatsPublishCallbackFunction)(void*);
struct AsyncNatsPublishCallback
{
  AsyncNatsPublishCallbackFunction f;  ///< @brief callback function
  void* closure;  ///< @brief data to be passed back
};
void async_nats_connection_publish_async(const AsyncNatsConnection* conn,
                                         AsyncNatsAsyncString topic,
                                         AsyncNatsAsyncMessage message,
                                         AsyncNatsPublishCallback cb);

/// -- Subscribe --
typedef void (*AsyncNatsSubscribeCallbackFunction)(AsyncNatsSubscription* sub,
                                                   AsyncNatsOwnedString err,
                                                   void*);
struct AsyncNatsSubscribeCallback
{
  AsyncNatsSubscribeCallbackFunction f;  ///< @brief callback function
  void* closure;  ///< @brief data to be passed back
};
void async_nats_connection_subscribe_async(const AsyncNatsConnection* conn,
                                           AsyncNatsAsyncString topic,
                                           AsyncNatsSubscribeCallback cb);

/// ---- NatsMessage ----
struct AsyncNatsMessage;
void async_nats_message_delete(AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_topic(const AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_data(const AsyncNatsMessage* msg);
AsyncNatsSlice async_nats_message_reply_to(const AsyncNatsMessage* msg);

/// ---- Subscription ----
void async_nats_subscribtion_delete(AsyncNatsSubscription* sub);
typedef void (*RecieveCallbackFunction)(AsyncNatsMessage* msg, void*);
struct AsyncNatsRecieveCallback
{
  RecieveCallbackFunction f;  ///< @brief callback function
  void* closure;  ///< @brief data to be passed back
};
void async_nats_subscribtion_receive_async(AsyncNatsSubscription* chan,
                                           AsyncNatsRecieveCallback cb);

/// ---- JetStream ----

EXTERN_C_END

#endif  // async_nats_CAPI_H
