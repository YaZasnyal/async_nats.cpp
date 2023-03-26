pub type NatsMessage = async_nats::Message;

use crate::api::{Slice};

/// Deletes NatsMessage.
/// Using this object after free causes undefined bahavior
#[no_mangle]
pub extern "C" fn async_nats_message_delete(msg: *mut NatsMessage) {
    unsafe {
        drop(Box::from_raw(msg));
    }
}

/// Returns C-string with topic that was used to publish this message
/// Topic is valid while NatsMessage is valid
#[no_mangle]
pub extern "C" fn async_nats_message_topic(msg: *const NatsMessage) -> Slice {
    let msg = unsafe { &*msg };
    Slice {
        data: msg.subject.as_ptr(),
        size: msg.subject.len() as u64,
    }
}

/// Returs Slice with payload details
/// Slice is valid while NatsMessage is valid
#[no_mangle]
pub extern "C" fn async_nats_message_data(msg: *const NatsMessage) -> Slice {
    let msg = unsafe { &*msg };
    Slice {
        data: msg.payload.as_ptr(),
        size: msg.payload.len() as u64,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_message_reply_to(msg: *const NatsMessage) -> Slice {
    let msg = unsafe { &*msg };

    let Some(reply) = &msg.reply else {
        return Slice::default();
    };

    Slice {
        data: reply.as_ptr(),
        size: reply.len() as u64,
    }
}