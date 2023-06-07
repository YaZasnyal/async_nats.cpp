use crate::api::AsyncNatsOwnedString;

// pub type AsyncNatsError = i32;

// pub type AsyncNatsIoError = AsyncNatsError;

// pub type AsyncNatsConnectionError = AsyncNatsError;

// use async_nats::{ConnectError, ConnectErrorKind};
pub struct AsyncNatsConnectError(pub async_nats::ConnectError);

#[repr(C)]
pub enum AsyncNatsConnectErrorKind {
    /// Parsing the passed server address failed.
    ServerParse,
    /// DNS related issues.
    Dns,
    /// Failed authentication process, signing nonce, etc.
    Authentication,
    /// Server returned authorization violation error.
    AuthorizationViolation,
    /// Connect timed out.
    TimedOut,
    /// Erroneous TLS setup.
    Tls,
    /// Other IO error.
    Io,
}

#[no_mangle]
pub extern "C" fn async_nats_connection_error_kind(
    err: *const AsyncNatsConnectError,
) -> AsyncNatsConnectErrorKind {
    let err = unsafe { &*err };
    match err.0.kind() {
        async_nats::ConnectErrorKind::ServerParse => AsyncNatsConnectErrorKind::ServerParse,
        async_nats::ConnectErrorKind::Dns => AsyncNatsConnectErrorKind::Dns,
        async_nats::ConnectErrorKind::Authentication => AsyncNatsConnectErrorKind::Authentication,
        async_nats::ConnectErrorKind::AuthorizationViolation => {
            AsyncNatsConnectErrorKind::AuthorizationViolation
        }
        async_nats::ConnectErrorKind::TimedOut => AsyncNatsConnectErrorKind::TimedOut,
        async_nats::ConnectErrorKind::Tls => AsyncNatsConnectErrorKind::Tls,
        async_nats::ConnectErrorKind::Io => AsyncNatsConnectErrorKind::Io,
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
    unsafe {
        drop(Box::from_raw(err));
    }
}
