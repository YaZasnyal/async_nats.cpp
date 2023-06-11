use crate::{
    api::{AsyncNatsAsyncMessage, AsyncNatsAsyncString, LossyConvert},
    connection::AsyncNatsConnection,
    error::AsyncNatsRequestError,
    message::AsyncNatsMessage,
};
use core::slice;
use std::ffi::c_void;

#[repr(C)]
#[derive(Debug, Clone)]
pub struct AsyncNatsRequestCallback(
    extern "C" fn(msg: *mut AsyncNatsMessage, err: *mut AsyncNatsRequestError, d: *mut c_void),
    *mut c_void,
);
unsafe impl Send for AsyncNatsRequestCallback {}

#[no_mangle]
pub extern "C" fn async_nats_connection_request_async(
    conn: *const AsyncNatsConnection,
    topic: AsyncNatsAsyncString,
    message: AsyncNatsAsyncMessage,
    cb: AsyncNatsRequestCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();

    conn.rt.spawn(async move {
        let cb = cb.clone();
        let message = message;
        let data_slice =
            unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
        // TODO: Same as async_nats_connection_publish_async
        let bytes = bytes::Bytes::copy_from_slice(data_slice);

        let response = conn.client.request(topic_str, bytes).await;
        match response {
            Ok(msg) => {
                let boxed_msg = Box::new(AsyncNatsMessage(msg));
                cb.0(Box::into_raw(boxed_msg), std::ptr::null_mut(), cb.1);
            }
            Err(err) => {
                let err = Box::new(AsyncNatsRequestError(err));
                cb.0(std::ptr::null_mut(), Box::leak(err), cb.1)
            },
        }
    });
}

#[no_mangle]
pub extern "C" fn async_nats_connection_send_request_async(
    conn: *const AsyncNatsConnection,
    topic: AsyncNatsAsyncString,
    request: *mut AsyncNatsRequest,
    cb: AsyncNatsRequestCallback,
) {
    let conn = unsafe { &*conn };
    let topic_str = topic.lossy_convert();
    let mut request = unsafe { Box::from_raw(request) };

    conn.rt.spawn(async move {
        let cb = cb.clone();
        let response = conn.client.send_request(topic_str, request.build()).await;
        match response {
            Ok(msg) => {
                let boxed_msg = Box::new(AsyncNatsMessage(msg));
                cb.0(Box::into_raw(boxed_msg), std::ptr::null_mut(), cb.1);
            }
            Err(err) => {
                let err = Box::new(AsyncNatsRequestError(err));
                cb.0(std::ptr::null_mut(), Box::leak(err), cb.1)
            },
        }
    });
}

#[derive(Default)]
pub struct AsyncNatsRequest {
    inbox: Option<String>,
    timeout: Option<core::time::Duration>,
    payload: Option<bytes::Bytes>,
    // TODO: headers
}

impl AsyncNatsRequest {
    pub fn build(&mut self) -> async_nats::Request {
        let mut req = async_nats::Request::new();
        if self.inbox.is_some() {
            req = req.inbox(self.inbox.take().unwrap());
        }
        if self.timeout.is_some() {
            req = req.timeout(self.timeout.take());
        }
        if self.payload.is_some() {
            req = req.payload(self.payload.take().unwrap());
        }
        req
    }
}

#[no_mangle]
pub extern "C" fn async_nats_request_new() -> *mut AsyncNatsRequest {
    Box::leak(Box::new(AsyncNatsRequest::default()))
}

#[no_mangle]
pub extern "C" fn async_nats_request_delete(req: *mut AsyncNatsRequest) {
    unsafe {
        drop(Box::from_raw(req));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_request_inbox(
    req: *mut AsyncNatsRequest,
    inbox: AsyncNatsAsyncString,
) {
    let req = unsafe { &mut *req };
    let inbox_str = inbox.lossy_convert();
    req.inbox = Some(inbox_str);
}

#[no_mangle]
pub extern "C" fn async_nats_request_timeout(req: *mut AsyncNatsRequest, timeout: u64) {
    let req = unsafe { &mut *req };
    req.timeout = Some(core::time::Duration::from_millis(timeout));
}

#[no_mangle]
pub extern "C" fn async_nats_request_message(
    req: *mut AsyncNatsRequest,
    message: AsyncNatsAsyncMessage,
) {
    let req = unsafe { &mut *req };
    let data_slice =
        unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
    // TODO: Same as async_nats_connection_publish_async
    let bytes = bytes::Bytes::copy_from_slice(data_slice);
    req.payload = Some(bytes);
}
