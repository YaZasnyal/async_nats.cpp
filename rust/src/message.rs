use std::sync::atomic::{AtomicU64, Ordering};

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
        data: msg.0.subject.as_ptr(),
        size: msg.0.subject.len() as u64,
    }
}

/// Returs Slice with payload details
/// Slice is valid while NatsMessage is valid
#[no_mangle]
pub extern "C" fn async_nats_message_data(msg: *const AsyncNatsMessage) -> AsyncNatsSlice {
    let msg = unsafe { &*msg };
    AsyncNatsSlice {
        data: msg.0.payload.as_ptr(),
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
        data: reply.as_ptr(),
        size: reply.len() as u64,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_to_string(
    msg: *const AsyncNatsMessage,
) -> AsyncNatsOwnedString {
    let msg = unsafe { &*msg };
    string_to_owned_string(format!("{:?}", &msg.0))
}
