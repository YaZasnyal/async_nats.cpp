use crate::api::{BorrowedMessage, BorrowedString, LossyConvert};
use crate::connection::Connection;
use bytes::Bytes;
use std::ffi::c_ulonglong;
use std::sync::Arc;
use tokio::sync::mpsc::{unbounded_channel, UnboundedSender};
use tokio::sync::{OwnedSemaphorePermit, Semaphore};

#[derive(Clone)]
pub struct NamedSender {
    inner: Arc<NamedSenderInner>,
}

impl NamedSender {
    pub fn new(topic: String, conn: &Connection) -> Self {
        Self::with_capacity(topic, conn, 1024)
    }

    pub fn with_capacity(topic: String, conn: &Connection, capacity: usize) -> Self {
        let (tx, mut rx) = unbounded_channel();

        let inner = Arc::new(NamedSenderInner {
            topic: topic,
            conn: conn.clone(),
            sender: tx,
            sem: Arc::new(Semaphore::new(capacity)),
        });

        let inner_clone = inner.clone();
        conn.rt.spawn(async move {
            loop {
                let Some(msg) = rx.recv().await else {
                    break;
                };

                inner_clone
                    .conn
                    .client
                    .publish(
                        msg.topic.unwrap_or_else(|| inner_clone.topic.clone()),
                        msg.message,
                    )
                    .await
                    .expect("Unknown error while sending event from the queue");
            }
        });

        Self { inner: inner }
    }
}

struct NamedSenderInner {
    topic: String,
    conn: Connection,
    sender: UnboundedSender<Message>,
    sem: Arc<Semaphore>,
}

struct Message {
    topic: Option<String>,
    message: Bytes,
    _permit: Option<OwnedSemaphorePermit>,
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_new(
    topic: BorrowedString,
    conn: *const Connection,
    capacity: c_ulonglong,
) -> *mut NamedSender {
    let conn = unsafe { &*conn };
    let sender = Box::new(NamedSender::with_capacity(
        topic.lossy_convert(),
        conn,
        capacity as usize,
    ));
    Box::into_raw(sender)
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_clone(sender: *const NamedSender) -> *mut NamedSender {
    let sender = unsafe { &*sender };
    let new_sender = Box::new(sender.clone());
    Box::into_raw(new_sender)
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_delete(sender: *mut NamedSender) {
    unsafe {
        drop(Box::from_raw(sender));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_try_send(
    sender: *const NamedSender,
    topic: BorrowedString,
    data: BorrowedMessage,
) -> bool {
    let sender = unsafe { &*sender };

    let permit = sender.inner.sem.clone().try_acquire_owned();
    let Ok(permit) = permit else {
        return false;
    };

    let data_slice =
        unsafe { std::slice::from_raw_parts(data.0 as *const u8, data.1.try_into().unwrap()) };
    let bytes = bytes::Bytes::copy_from_slice(data_slice);
    let message = Message {
        topic: if !topic.is_null() {
            Some(topic.lossy_convert())
        } else {
            None
        },
        message: bytes,
        _permit: Some(permit),
    };
    sender.inner.sender.send(message).ok();
    true
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_send(
    sender: *const NamedSender,
    topic: BorrowedString,
    data: BorrowedMessage,
) {
    let sender = unsafe { &*sender };
    let permit = sender.inner.sem.clone().try_acquire_owned();
    let permit = match permit {
        Ok(x) => Some(x),
        Err(_) => None, // send without a permit
    };

    let data_slice =
        unsafe { std::slice::from_raw_parts(data.0 as *const u8, data.1.try_into().unwrap()) };
    let bytes = bytes::Bytes::copy_from_slice(data_slice);
    let message = Message {
        topic: if !topic.is_null() {
            Some(topic.lossy_convert())
        } else {
            None
        },
        message: bytes,
        _permit: permit,
    };
    sender.inner.sender.send(message).ok();
}
