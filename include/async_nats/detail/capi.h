#ifndef async_nats_CAPI_H
#define async_nats_CAPI_H

/* Generated with cbindgen:0.24.3 */

/* Warning, this file is autogenerated by cbindgen. Don't modify this manually. */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


typedef enum AsyncNatsConnectErrorKind
{
  /**
   * Parsing the passed server address failed.
   */
  ServerParse,
  /**
   * DNS related issues.
   */
  Dns,
  /**
   * Failed authentication process, signing nonce, etc.
   */
  Authentication,
  /**
   * Server returned authorization violation error.
   */
  AuthorizationViolation,
  /**
   * Connect timed out.
   */
  TimedOut,
  /**
   * Erroneous TLS setup.
   */
  Tls,
  /**
   * Other IO error.
   */
  Io,
} AsyncNatsConnectErrorKind;

typedef struct AsyncNatsConnectError AsyncNatsConnectError;

typedef struct AsyncNatsConnection AsyncNatsConnection;

typedef struct AsyncNatsConnetionParams AsyncNatsConnetionParams;

typedef struct AsyncNatsMessage AsyncNatsMessage;

typedef struct AsyncNatsNamedReceiver AsyncNatsNamedReceiver;

typedef struct AsyncNatsNamedSender AsyncNatsNamedSender;

typedef struct AsyncNatsRuntimeConfig AsyncNatsRuntimeConfig;

typedef struct AsyncNatsSubscribtion AsyncNatsSubscribtion;

typedef struct AsyncNatsTokioRuntime AsyncNatsTokioRuntime;

typedef struct AsyncNatsTokioRuntimeConfig AsyncNatsTokioRuntimeConfig;

/**
 * BorrowedString is a C-string with lifetime limited to a specific function call
 * or while object that produced this string is valid
 */
typedef const char *AsyncNatsBorrowedString;

typedef struct AsyncNatsConnectCallback
{
  void (*_0)(struct AsyncNatsConnection *conn, struct AsyncNatsConnectError *err, void *closure);
  void *_1;
} AsyncNatsConnectCallback;

typedef char *AsyncNatsOwnedString;

typedef AsyncNatsBorrowedString AsyncNatsAsyncString;

/**
 * BorrowedMessage represents a byte stream with a lifetime limited to a
 * specific function call.
 */
typedef struct AsyncNatsBorrowedMessage
{
  const char *_0;
  unsigned long long _1;
} AsyncNatsBorrowedMessage;

/**
 * AsyncMessage represents a byte stream with a lifetime limited to a
 * specific async call and should be valid until a callback is called
 */
typedef struct AsyncNatsBorrowedMessage AsyncNatsAsyncMessage;

typedef struct AsyncNatsPublishCallback
{
  void (*_0)(void *d);
  void *_1;
} AsyncNatsPublishCallback;

typedef struct AsyncNatsSubscribeCallback
{
  void (*_0)(struct AsyncNatsSubscribtion *sub, AsyncNatsOwnedString err, void *d);
  void *_1;
} AsyncNatsSubscribeCallback;

typedef struct AsyncNatsSlice
{
  const uint8_t *data;
  uint64_t size;
} AsyncNatsSlice;

typedef struct AsyncNatsReceiveCallback
{
  void (*_0)(struct AsyncNatsMessage *m, void *c);
  void *_1;
} AsyncNatsReceiveCallback;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct AsyncNatsConnection *async_nats_connection_clone(const struct AsyncNatsConnection *conn);

void async_nats_connection_config_addr(struct AsyncNatsConnetionParams *cfg,
                                       AsyncNatsBorrowedString addr);

void async_nats_connection_config_delete(struct AsyncNatsConnetionParams *cfg);

void async_nats_connection_config_name(struct AsyncNatsConnetionParams *cfg,
                                       AsyncNatsBorrowedString name);

struct AsyncNatsConnetionParams *async_nats_connection_config_new(void);

void async_nats_connection_connect(const struct AsyncNatsTokioRuntime *rt,
                                   const struct AsyncNatsConnetionParams *cfg,
                                   struct AsyncNatsConnectCallback cb);

void async_nats_connection_delete(struct AsyncNatsConnection *conn);

void async_nats_connection_error_delete(struct AsyncNatsConnectError *err);

AsyncNatsOwnedString async_nats_connection_error_describtion(const struct AsyncNatsConnectError *err);

enum AsyncNatsConnectErrorKind async_nats_connection_error_kind(const struct AsyncNatsConnectError *err);

/**
 * Publish data asynchronously.
 *
 * topic and message: must be valid until callback is called.
 */
void async_nats_connection_publish_async(const struct AsyncNatsConnection *conn,
                                         AsyncNatsAsyncString topic,
                                         AsyncNatsAsyncMessage message,
                                         struct AsyncNatsPublishCallback cb);

void async_nats_connection_subscribe_async(const struct AsyncNatsConnection *conn,
                                           AsyncNatsAsyncString topic,
                                           struct AsyncNatsSubscribeCallback cb);

/**
 * Returs Slice with payload details
 * Slice is valid while NatsMessage is valid
 */
struct AsyncNatsSlice async_nats_message_data(const struct AsyncNatsMessage *msg);

/**
 * Deletes NatsMessage.
 * Using this object after free causes undefined bahavior
 */
void async_nats_message_delete(struct AsyncNatsMessage *msg);

struct AsyncNatsSlice async_nats_message_reply_to(const struct AsyncNatsMessage *msg);

/**
 * Returns C-string with topic that was used to publish this message
 * Topic is valid while NatsMessage is valid
 */
struct AsyncNatsSlice async_nats_message_topic(const struct AsyncNatsMessage *msg);

void async_nats_named_receiver_delete(struct AsyncNatsNamedReceiver *recv);

struct AsyncNatsNamedReceiver *async_nats_named_receiver_new(struct AsyncNatsSubscribtion *s,
                                                             unsigned long long capacity);

struct AsyncNatsMessage *async_nats_named_receiver_recv(const struct AsyncNatsNamedReceiver *s);

struct AsyncNatsMessage *async_nats_named_receiver_try_recv(const struct AsyncNatsNamedReceiver *s);

struct AsyncNatsNamedSender *async_nats_named_sender_clone(const struct AsyncNatsNamedSender *sender);

void async_nats_named_sender_delete(struct AsyncNatsNamedSender *sender);

struct AsyncNatsNamedSender *async_nats_named_sender_new(AsyncNatsBorrowedString topic,
                                                         const struct AsyncNatsConnection *conn,
                                                         unsigned long long capacity);

void async_nats_named_sender_send(const struct AsyncNatsNamedSender *sender,
                                  AsyncNatsBorrowedString topic,
                                  struct AsyncNatsBorrowedMessage data);

bool async_nats_named_sender_try_send(const struct AsyncNatsNamedSender *sender,
                                      AsyncNatsBorrowedString topic,
                                      struct AsyncNatsBorrowedMessage data);

void async_nats_owned_string_delete(AsyncNatsOwnedString s);

void async_nats_subscribtion_delete(struct AsyncNatsSubscribtion *s);

void async_nats_subscribtion_receive_async(struct AsyncNatsSubscribtion *s,
                                           struct AsyncNatsReceiveCallback cb);

void async_nats_tokio_runtime_config_delete(struct AsyncNatsTokioRuntimeConfig *cfg);

struct AsyncNatsTokioRuntimeConfig *async_nats_tokio_runtime_config_new(void);

void async_nats_tokio_runtime_config_thread_count(struct AsyncNatsTokioRuntimeConfig *cfg,
                                                  uint32_t thread_count);

void async_nats_tokio_runtime_config_thread_name(struct AsyncNatsTokioRuntimeConfig *cfg,
                                                 AsyncNatsBorrowedString thread_name);

void async_nats_tokio_runtime_delete(struct AsyncNatsTokioRuntime *runtime);

struct AsyncNatsTokioRuntime *async_nats_tokio_runtime_new(const struct AsyncNatsTokioRuntimeConfig *cfg);

void nats_runtime_config_free(struct AsyncNatsRuntimeConfig *cfg);

struct AsyncNatsRuntimeConfig *nats_runtime_config_new(void);

void nats_runtime_config_set_endpoint(struct AsyncNatsRuntimeConfig *cfg, const char *endpoint);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* async_nats_CAPI_H */
