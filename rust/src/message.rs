
pub struct AsyncNatsMessage(pub async_nats::Message);

impl Into<AsyncNatsMessage> for async_nats::Message {
    fn into(self) -> AsyncNatsMessage {
        AsyncNatsMessage {0: self}
    }
}

use crate::api::AsyncNatsSlice;

/// Deletes NatsMessage.
/// Using this object after free causes undefined bahavior
#[no_mangle]
pub extern "C" fn async_nats_message_delete(msg: *mut AsyncNatsMessage) {
    unsafe {
        drop(Box::from_raw(msg));
    }
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
