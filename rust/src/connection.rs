use crate::tokio_runtime::TokioRuntime;

use crate::api::{AsyncMessage, AsyncString, BorrowedString, LossyConvert, OwnedString};
use crate::subscribtion::SubscribtionWrapper;
use async_nats::{connect_with_options, Client, ConnectOptions, ServerAddr};
use core::slice;
use std::ffi::c_void;

#[derive(Clone)]
pub struct Connection {
    pub(crate) rt: tokio::runtime::Handle,
    pub(crate) client: Client,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct ConnectCallback(
    extern "C" fn(conn: *mut Connection, err: i32, closure: *mut c_void),
    *mut c_void,
);
unsafe impl Send for ConnectCallback {}

#[no_mangle]
pub extern "C" fn async_nats_connection_connect(
    rt: *const TokioRuntime,
    cfg: *const ConnetionParams,
    cb: ConnectCallback,
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
                // TODO: Error kind got changed. This should be fixed later;
                // Printing to stderr for now
                eprintln!("Connection error: {}", err.to_string());
                cb.0(std::ptr::null_mut(), 0 as i32, cb.1);
                return;
            }
        };

        let conn = Box::new(Connection {
            rt: handle,
            client: conn,
        });

        cb.0(Box::into_raw(conn), 0, cb.1);
    });
}

#[no_mangle]
pub extern "C" fn async_nats_connection_clone(conn: *const Connection) -> *mut Connection {
    let conn = unsafe { &*conn };
    let new_conn = Box::new(conn.clone());
    Box::into_raw(new_conn)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_delete(conn: *mut Connection) {
    unsafe {
        drop(Box::from_raw(conn));
    }
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct PublishCallback(extern "C" fn(d: *mut c_void), *mut c_void);
unsafe impl Send for PublishCallback {}

/// Publish data asynchronously.
///
/// topic and message: must be valid until callback is called.
#[no_mangle]
pub extern "C" fn async_nats_connection_publish_async(
    conn: *const Connection,
    topic: AsyncString,
    message: AsyncMessage,
    cb: PublishCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();

    conn.rt.spawn(async move {
        let cb = cb.clone();
        let message = message;
        let data_slice =
            unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
        // Have to copy because there is no way to know when bytes object is dropped to
        // call the callback.
        // Should wait for `https://github.com/tokio-rs/bytes/issues/437` and think for
        // better solution depending on implementation
        let bytes = bytes::Bytes::copy_from_slice(data_slice);
        conn.client.publish(topic_str, bytes).await.ok();
        cb.0(cb.1);
    });
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct SubscribeCallback(
    extern "C" fn(sub: *mut SubscribtionWrapper, err: OwnedString, d: *mut c_void),
    *mut c_void,
);
unsafe impl Send for SubscribeCallback {}

#[no_mangle]
extern "C" fn async_nats_connection_subscribe_async(
    conn: *const Connection,
    topic: AsyncString,
    cb: SubscribeCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();

    let rt = conn.rt.clone();
    conn.rt.spawn(async move {
        let cb = cb;
        let sub = conn.client.subscribe(topic_str).await;

        match sub {
            Ok(sub) => {
                let sub = Box::new(SubscribtionWrapper::new(rt, sub));
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
pub struct ConnetionParams {
    addrs: Vec<ServerAddr>,
    name: Option<String>,
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_new() -> *mut ConnetionParams {
    let cfg = Box::new(ConnetionParams::default());
    Box::into_raw(cfg)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_delete(cfg: *mut ConnetionParams) {
    unsafe {
        drop(Box::from_raw(cfg));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_name(
    cfg: *mut ConnetionParams,
    name: BorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.name = Some(name.lossy_convert());
}

#[no_mangle]
pub extern "C" fn async_nats_connection_config_addr(
    cfg: *mut ConnetionParams,
    addr: BorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    // TODO: Add proper error handling
    cfg.addrs.push(
        addr.lossy_convert()
            .parse()
            .expect("Unable to parse address"),
    );
}
