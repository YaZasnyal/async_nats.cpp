use crate::error::AsyncNatsConnectError;
use crate::tokio_runtime::AsyncNatsTokioRuntime;
use bytes::{BytesMut};
use std::cell::RefCell;

use crate::api::{
    AsyncNatsAsyncMessage, AsyncNatsAsyncString, AsyncNatsBorrowedString, AsyncNatsOwnedString,
    AsyncNatsSlice, LossyConvert,
};
use crate::subscribtion::AsyncNatsSubscribtion;
use async_nats::{connect_with_options, Client, ConnectOptions, ServerAddr};
use core::slice;
use std::ffi::c_void;

#[derive(Clone)]
pub struct AsyncNatsConnection {
    pub(crate) rt: tokio::runtime::Handle,
    pub(crate) client: Client,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct AsyncNatsConnectCallback(
    extern "C" fn(
        conn: *mut AsyncNatsConnection,
        err: *mut crate::error::AsyncNatsConnectError,
        closure: *mut c_void,
    ),
    *mut c_void,
);
unsafe impl Send for AsyncNatsConnectCallback {}

#[no_mangle]
pub extern "C" fn async_nats_connection_connect(
    rt: *const AsyncNatsTokioRuntime,
    cfg: *const AsyncNatsConnetionParams,
    cb: AsyncNatsConnectCallback,
) {
    let rt = unsafe { &*rt };
    let cfg = unsafe { &*cfg };

    let handle = rt.handle().clone();
    rt.handle().spawn(async move {
        let cb = cb;
        let mut co = ConnectOptions::new();
        if let Some(name) = &cfg.name {
            co = co.name(name);
        }

        let conn = connect_with_options(cfg.addrs.clone(), co).await;
        let conn = match conn {
            Ok(conn) => conn,
            Err(err) => {
                let err = Box::new(AsyncNatsConnectError(err));
                cb.0(std::ptr::null_mut(), Box::leak(err), cb.1);
                return;
            }
        };

        let conn = Box::new(AsyncNatsConnection {
            rt: handle,
            client: conn,
        });

        cb.0(Box::into_raw(conn), std::ptr::null_mut(), cb.1);
    });
}

#[no_mangle]
pub extern "C" fn async_nats_connection_clone(
    conn: *const AsyncNatsConnection,
) -> *mut AsyncNatsConnection {
    let conn = unsafe { &*conn };
    let new_conn = Box::new(conn.clone());
    Box::into_raw(new_conn)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_delete(conn: *mut AsyncNatsConnection) {
    unsafe {
        drop(Box::from_raw(conn));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_mailbox(
    conn: *mut AsyncNatsConnection,
) -> AsyncNatsOwnedString {
    let conn = unsafe { &*conn };
    let mailbox = conn.client.new_inbox();
    crate::api::string_to_owned_string(mailbox)
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct AsyncNatsPublishCallback(extern "C" fn(d: *mut c_void), *mut c_void);
unsafe impl Send for AsyncNatsPublishCallback {}

/// Publish data asynchronously.
///
/// topic and message: must be valid until callback is called.
#[no_mangle]
pub extern "C" fn async_nats_connection_publish_async(
    conn: *const AsyncNatsConnection,
    topic: AsyncNatsSlice,
    message: AsyncNatsAsyncMessage,
    cb: AsyncNatsPublishCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();
    let data_slice =
        unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };

    thread_local! {
        static BYTES: RefCell<BytesMut> = RefCell::new(bytes::BytesMut::with_capacity(65535));
    }

    // Have to copy because there is no way to know when bytes object is dropped to
    // call the callback.
    // Should wait for `https://github.com/tokio-rs/bytes/issues/437` and think for
    // better solution depending on implementation
    let bytes = BYTES.with(|f| {
        let mut mbytes = f.borrow_mut();
        mbytes.extend_from_slice(data_slice);
        mbytes.split_to(data_slice.len()).freeze()
    });

    conn.rt.spawn(async move {
        let cb = cb.clone();
        conn.client.publish(topic_str, bytes).await.ok();
        cb.0(cb.1);
    });
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct AsyncNatsSubscribeCallback(
    extern "C" fn(sub: *mut AsyncNatsSubscribtion, err: AsyncNatsOwnedString, d: *mut c_void),
    *mut c_void,
);
unsafe impl Send for AsyncNatsSubscribeCallback {}

#[no_mangle]
pub extern "C" fn async_nats_connection_subscribe_async(
    conn: *const AsyncNatsConnection,
    topic: AsyncNatsAsyncString,
    cb: AsyncNatsSubscribeCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();

    let rt = conn.rt.clone();
    conn.rt.spawn(async move {
        let cb = cb;
        let sub = conn.client.subscribe(topic_str).await;

        match sub {
            Ok(sub) => {
                let sub = Box::new(AsyncNatsSubscribtion::new(rt, sub));
                cb.0(Box::into_raw(sub), std::ptr::null_mut(), cb.1);
            }
            Err(e) => {
                // TODO: replace with more specific error when it is implemented in async_nats.rs
                let err = std::ffi::CString::new(e.to_string().as_bytes())
                    .expect("Unable to convert error into CString");
                cb.0(std::ptr::null_mut(), std::ffi::CString::into_raw(err), cb.1);
            }
        }
    });
}

// ---- Config ----

#[derive(Default)]
pub struct AsyncNatsConnetionParams {
    addrs: Vec<ServerAddr>,
    name: Option<String>,
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_new() -> *mut AsyncNatsConnetionParams {
    let cfg = Box::new(AsyncNatsConnetionParams::default());
    Box::into_raw(cfg)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_delete(cfg: *mut AsyncNatsConnetionParams) {
    unsafe {
        drop(Box::from_raw(cfg));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_name(
    cfg: *mut AsyncNatsConnetionParams,
    name: AsyncNatsBorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.name = Some(name.lossy_convert());
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_addr(
    cfg: *mut AsyncNatsConnetionParams,
    addr: AsyncNatsBorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    // TODO: Add proper error handling
    cfg.addrs.push(
        addr.lossy_convert()
            .parse()
            .expect("Unable to parse address"),
    );
}
