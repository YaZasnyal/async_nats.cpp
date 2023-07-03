use std::{
    str::FromStr,
    sync::atomic::{AtomicU64, Ordering},
};
use std::ffi::c_void;

pub struct AsyncNatsMessage(pub async_nats::Message, pub(crate) AtomicU64);

impl Into<AsyncNatsMessage> for async_nats::Message {
    fn into(self) -> AsyncNatsMessage {
        AsyncNatsMessage {
            0: self,
            1: AtomicU64::new(1),
        }
    }
}

use crate::api::{string_to_owned_string, AsyncNatsOwnedString, AsyncNatsSlice};

/// Deletes NatsMessage.
/// Using this object after free causes undefined bahavior
#[no_mangle]
pub extern "C" fn async_nats_message_delete(msg: *mut AsyncNatsMessage) {
    let msg = unsafe { &mut *msg };
    let refs = msg.1.fetch_sub(1, Ordering::Release);
    if refs == 1 {
        unsafe {
            drop(Box::from_raw(msg));
        }
    }
}

/// Increments reference counter
#[no_mangle]
pub extern "C" fn async_nats_message_clone(msg: *mut AsyncNatsMessage) -> *mut AsyncNatsMessage {
    let msg_ref = unsafe { &*msg };
    msg_ref.1.fetch_add(1, Ordering::Acquire);
    msg
}

/// Returns C-string with topic that was used to publish this message
/// Topic is valid while NatsMessage is valid
#[no_mangle]
pub extern "C" fn async_nats_message_topic(msg: *const AsyncNatsMessage) -> AsyncNatsSlice {
    let msg = unsafe { &*msg };
    AsyncNatsSlice {
        data: msg.0.subject.as_ptr() as *const c_void,
        size: msg.0.subject.len() as u64,
    }
}

/// Returs Slice with payload details
/// Slice is valid while NatsMessage is valid
#[no_mangle]
pub extern "C" fn async_nats_message_data(msg: *const AsyncNatsMessage) -> AsyncNatsSlice {
    let msg = unsafe { &*msg };
    AsyncNatsSlice {
        data: msg.0.payload.as_ptr() as *const c_void,
        size: msg.0.payload.len() as u64,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_reply_to(msg: *const AsyncNatsMessage) -> AsyncNatsSlice {
    let msg = unsafe { &*msg };

    let Some(reply) = &msg.0.reply else {
        return AsyncNatsSlice::default();
    };

    AsyncNatsSlice {
        data: reply.as_ptr() as *const c_void,
        size: reply.len() as u64,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_has_headers(msg: *mut AsyncNatsMessage) -> bool {
    let msg_ref = unsafe { &*msg };
    msg_ref.0.headers.is_some()
}

struct HeaderIterator<'a> {
    iter: Option<
        std::collections::hash_map::Iter<'a, async_nats::HeaderName, async_nats::HeaderValue>,
    >,
    val: Option<(&'a async_nats::HeaderName, &'a async_nats::HeaderValue)>,
    refcnt: usize,
}

pub struct AsyncNatsHeaderIterator {
    _priv: [u8; 0],
    // Optional; to get the `Send` & `Sync` bounds right.
    _send_sync: ::core::marker::PhantomData<HeaderIterator<'static>>,
}

unsafe fn header_iterator_from_ptr<'_0, '_1>(
    ptr: *mut AsyncNatsHeaderIterator,
) -> &'_0 mut HeaderIterator<'_1> {
    if ptr.is_null() {
        eprintln!("Fatal error, got NULL `AsyncNatsHeaderIterator` pointer");
        ::std::process::abort();
    }
    &mut *(ptr.cast())
}

/// header iterator must live longer that AsyncNatsMessage
#[no_mangle]
pub extern "C" fn async_nats_message_header_iterator(
    msg: *const AsyncNatsMessage,
) -> *mut AsyncNatsHeaderIterator {
    let msg_ref = unsafe { &*msg };
    let headers = msg_ref
        .0
        .headers
        .as_ref()
        .expect("Headers are empty. Check HeadersView before using it");

    let hi = HeaderIterator {
        iter: Some(headers.iter()),
        val: Option::None,
        refcnt: 1,
    };
    Box::into_raw(Box::new(hi)).cast()
}

#[no_mangle]
pub unsafe extern "C" fn async_nats_message_header_iterator_copy(p: *mut AsyncNatsHeaderIterator) {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    ctx.refcnt += 1;
}

#[no_mangle]
pub unsafe extern "C" fn async_nats_message_header_iterator_free(p: *mut AsyncNatsHeaderIterator) {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    ctx.refcnt -= 1;
    if ctx.refcnt == 0 {
        drop::<Box<HeaderIterator<'_>>>(Box::from_raw(header_iterator_from_ptr(p)))
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_header_iterator_next(p: *mut AsyncNatsHeaderIterator) -> bool {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    let val = ctx
        .iter
        .as_mut()
        .expect("Iter must be valid. This value must be checked on C++ side")
        .next();
    ctx.val = val;
    return ctx.val.is_some();
}

#[no_mangle]
pub extern "C" fn async_nats_message_header_iterator_key(
    p: *mut AsyncNatsHeaderIterator,
) -> AsyncNatsSlice {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    let (k, _) = ctx.val.expect("Trying to derev an empty header");
    let str: &str = k.as_ref();
    AsyncNatsSlice {
        data: str.as_ptr() as *const c_void,
        size: str.len() as u64,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_get_header(
    msg: *const AsyncNatsMessage,
    header: AsyncNatsSlice,
) -> *mut AsyncNatsHeaderIterator {
    let msg = unsafe { &*msg };
    let Some(headers) = &msg.0.headers else {
        return std::ptr::null_mut();
    };

    let Ok(h) = async_nats::HeaderName::from_str(
        header
            .as_str()
            .expect("Header is empty. Validity must be checked on C++ side"),
    ) else {
        return std::ptr::null_mut();
    };
    let Some(v) = headers.get(h) else {
        return std::ptr::null_mut();
    };

    static DUMMY: async_nats::HeaderName = async_nats::HeaderName::from_static("dummy_header");
    let hi = HeaderIterator {
        iter: Default::default(),
        val: Some((&DUMMY, v)),
        refcnt: 1,
    };
    return Box::into_raw(Box::new(hi)).cast();
}

#[no_mangle]
pub extern "C" fn async_nats_message_header_iterator_value_count(
    p: *mut AsyncNatsHeaderIterator,
) -> u64 {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    let (_, v) = ctx.val.expect("Trying to derev an empty header");
    v.iter().count() as u64
}

#[no_mangle]
pub extern "C" fn async_nats_message_header_iterator_value_at(
    p: *mut AsyncNatsHeaderIterator,
    index: u64,
) -> AsyncNatsSlice {
    let ctx: &'_ mut HeaderIterator<'_> = unsafe { header_iterator_from_ptr(p) };
    let (_, v) = ctx.val.expect("Trying to derev an empty header");
    let str = v
        .iter()
        .skip(index as usize)
        .next()
        .expect("Index is out of range. Must be checked on C++ side");

    AsyncNatsSlice {
        data: str.as_ptr() as *const c_void,
        size: str.len() as u64,
    }
}

#[repr(C)]
#[allow(non_camel_case_types, unused)]
pub enum AsyncNatsMessageStatus {
    AsyncNatsMessageStatus_None = 0,
    AsyncNatsMessageStatus_IdleHeartbeat = 100,
    AsyncNatsMessageStatus_Ok = 200,
    AsyncNatsMessageStatus_NotFound = 404,
    AsyncNatsMessageStatus_Timeout = 408,
    AsyncNatsMessageStatus_NoResponders = 503,
    AsyncNatsMessageStatus_RequestTerminated = 409,
}

/// Optional Status of the message. Used mostly for internal handling
///
/// =0 - no status code
/// >0 - some status code
#[no_mangle]
pub extern "C" fn async_nats_message_status(msg: *const AsyncNatsMessage) -> u16 {
    let msg = unsafe { &*msg };

    let Some(status) = &msg.0.status else {
        return 0;
    };

    status.as_u16()
}

#[no_mangle]
pub extern "C" fn async_nats_message_description(msg: *const AsyncNatsMessage) -> AsyncNatsSlice {
    let msg = unsafe { &*msg };

    let Some(description) = &msg.0.description else {
        return AsyncNatsSlice::default();
    };

    AsyncNatsSlice {
        data: description.as_ptr() as *const c_void,
        size: description.len() as u64,
    }
}

/// Return length of the message over the wire
#[no_mangle]
pub extern "C" fn async_nats_message_length(msg: *const AsyncNatsMessage) -> u64 {
    let msg = unsafe { &*msg };
    msg.0.length as u64
}

#[no_mangle]
pub extern "C" fn async_nats_message_to_string(
    msg: *const AsyncNatsMessage,
) -> AsyncNatsOwnedString {
    let msg = unsafe { &*msg };
    string_to_owned_string(format!("{:?}", &msg.0))
}
