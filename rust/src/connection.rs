use crate::tokio_runtime::TokioRuntime;

use crate::api::{AsyncMessage, AsyncString, BorrowedString, LossyConvert};
use crate::error::io_error_convert;
use async_nats::{connect_with_options, Client, ConnectOptions, ServerAddr};
use core::slice;
use std::ffi::{c_char, c_ulonglong, c_void};

#[derive(Clone)]
pub struct Connection {
    rt: tokio::runtime::Handle,
    client: Client,
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

    rt.handle().clone().spawn(async move {
        let cb = cb;
        let mut co = ConnectOptions::new();
        if let Some(name) = &cfg.name {
            co = co.name(name);
        }

        let conn = connect_with_options(cfg.addrs.clone(), co).await;
        let conn = match conn {
            Ok(conn) => conn,
            Err(err) => {
                cb.0(std::ptr::null_mut(), io_error_convert(err), cb.1);
                return;
            }
        };

        let conn = Box::new(Connection {
            rt: rt.handle().clone(),
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
    let data_slice =
        unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
    let bytes = bytes::Bytes::from_static(data_slice);

    conn.rt.spawn(async move {
        let _data_slice = data_slice;
        let cb = cb.clone();
        conn.client.publish(topic_str, bytes).await.ok();
        cb.0(cb.1);
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
