use crate::api::AsyncNatsOwnedString;
use std::sync::atomic::{AtomicUsize, Ordering};

// ------------------- ConnectError -------------------
pub struct AsyncNatsConnectError(pub async_nats::ConnectError, AtomicUsize);

impl AsyncNatsConnectError {
    pub fn new(e: async_nats::ConnectError) -> Self {
        Self {
            0: e,
            1: AtomicUsize::new(1),
        }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_error_clone(
    err: *mut AsyncNatsConnectError,
) -> *mut AsyncNatsConnectError {
    let err_ptr = unsafe { &*err };
    err_ptr.1.fetch_add(1, std::sync::atomic::Ordering::Acquire);
    return err;
}

#[repr(C)]
#[allow(non_camel_case_types)]
pub enum AsyncNatsConnectErrorKind {
    /// Parsing the passed server address failed.
    AsyncNats_Connect_ServerParse,
    /// DNS related issues.
    AsyncNats_Connect_Dns,
    /// Failed authentication process, signing nonce, etc.
    AsyncNats_Connect_Authentication,
    /// Server returned authorization violation error.
    AsyncNats_Connect_AuthorizationViolation,
    /// Connect timed out.
    AsyncNats_Connect_TimedOut,
    /// Erroneous TLS setup.
    AsyncNatsConnectTls,
    /// Other IO error.
    AsyncNats_ConnectIo,
}

#[no_mangle]
pub extern "C" fn async_nats_connection_error_kind(
    err: *const AsyncNatsConnectError,
) -> AsyncNatsConnectErrorKind {
    let err = unsafe { &*err };
    match err.0.kind() {
        async_nats::ConnectErrorKind::ServerParse => {
            AsyncNatsConnectErrorKind::AsyncNats_Connect_ServerParse
        }
        async_nats::ConnectErrorKind::Dns => AsyncNatsConnectErrorKind::AsyncNats_Connect_Dns,
        async_nats::ConnectErrorKind::Authentication => {
            AsyncNatsConnectErrorKind::AsyncNats_Connect_Authentication
        }
        async_nats::ConnectErrorKind::AuthorizationViolation => {
            AsyncNatsConnectErrorKind::AsyncNats_Connect_AuthorizationViolation
        }
        async_nats::ConnectErrorKind::TimedOut => {
            AsyncNatsConnectErrorKind::AsyncNats_Connect_TimedOut
        }
        async_nats::ConnectErrorKind::Tls => AsyncNatsConnectErrorKind::AsyncNatsConnectTls,
        async_nats::ConnectErrorKind::Io => AsyncNatsConnectErrorKind::AsyncNats_ConnectIo,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_error_describtion(
    err: *const AsyncNatsConnectError,
) -> AsyncNatsOwnedString {
    let err = unsafe { &*err };
    crate::api::string_to_owned_string(err.0.to_string())
}

#[no_mangle]
pub extern "C" fn async_nats_connection_error_delete(err: *mut AsyncNatsConnectError) {
    let err_ptr = unsafe { &*err };
    let c = err_ptr.1.fetch_sub(1, Ordering::Release);
    if c == 1 {
        unsafe {
            drop(Box::from_raw(err));
        }
    }
}

// ------------------- RequestError -------------------

pub struct AsyncNatsRequestError(pub async_nats::RequestError, AtomicUsize);

impl AsyncNatsRequestError {
    pub fn new(e: async_nats::RequestError) -> Self {
        Self {
            0: e,
            1: AtomicUsize::new(1),
        }
    }
}

#[repr(C)]
#[allow(non_camel_case_types)]
pub enum AsyncNatsRequestErrorKind {
    /// There are services listening on requested subject, but they didn't respond
    /// in time.
    AsyncNats_Request_TimedOut,
    /// No one is listening on request subject.
    AsyncNats_Request_NoResponders,
    /// Other errors, client/io related.
    AsyncNats_Request_Other,
}

#[no_mangle]
pub extern "C" fn async_nats_request_error_clone(
    err: *mut AsyncNatsRequestError,
) -> *mut AsyncNatsRequestError {
    let err_ptr = unsafe { &*err };
    err_ptr.1.fetch_add(1, std::sync::atomic::Ordering::Acquire);
    return err;
}

#[no_mangle]
pub extern "C" fn async_nats_request_error_kind(
    err: *const AsyncNatsRequestError,
) -> AsyncNatsRequestErrorKind {
    let err = unsafe { &*err };
    match err.0.kind() {
        async_nats::RequestErrorKind::TimedOut => {
            AsyncNatsRequestErrorKind::AsyncNats_Request_TimedOut
        }
        async_nats::RequestErrorKind::NoResponders => {
            AsyncNatsRequestErrorKind::AsyncNats_Request_NoResponders
        }
        async_nats::RequestErrorKind::Other => AsyncNatsRequestErrorKind::AsyncNats_Request_Other,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_request_error_describtion(
    err: *const AsyncNatsRequestError,
) -> AsyncNatsOwnedString {
    let err = unsafe { &*err };
    crate::api::string_to_owned_string(err.0.to_string())
}

#[no_mangle]
pub extern "C" fn async_nats_request_error_delete(err: *mut AsyncNatsRequestError) {
    let err_ptr = unsafe { &*err };
    let c = err_ptr.1.fetch_sub(1, Ordering::Release);
    if c == 1 {
        unsafe {
            drop(Box::from_raw(err));
        }
    }
}
