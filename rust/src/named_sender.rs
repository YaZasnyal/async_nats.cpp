use crate::api::{AsyncNatsBorrowedMessage, AsyncNatsBorrowedString, LossyConvert};
use crate::connection::AsyncNatsConnection;
use bytes::{Bytes, BytesMut};
use std::cell::RefCell;
use std::ffi::c_ulonglong;
use std::sync::Arc;
use tokio::sync::mpsc::{unbounded_channel, UnboundedSender};
use tokio::sync::{OwnedSemaphorePermit, Semaphore};

#[derive(Clone)]
pub struct AsyncNatsNamedSender {
    inner: Arc<NamedSenderInner>,
}

impl AsyncNatsNamedSender {
    pub fn with_capacity(topic: String, conn: &AsyncNatsConnection, capacity: usize) -> Self {
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
    conn: AsyncNatsConnection,
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
    topic: AsyncNatsBorrowedString,
    conn: *const AsyncNatsConnection,
    capacity: c_ulonglong,
) -> *mut AsyncNatsNamedSender {
    let conn = unsafe { &*conn };
    let sender = Box::new(AsyncNatsNamedSender::with_capacity(
        topic.lossy_convert(),
        conn,
        capacity as usize,
    ));
    Box::into_raw(sender)
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_clone(
    sender: *const AsyncNatsNamedSender,
) -> *mut AsyncNatsNamedSender {
    let sender = unsafe { &*sender };
    let new_sender = Box::new(sender.clone());
    Box::into_raw(new_sender)
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_delete(sender: *mut AsyncNatsNamedSender) {
    unsafe {
        drop(Box::from_raw(sender));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_named_sender_try_send(
    sender: *const AsyncNatsNamedSender,
    topic: AsyncNatsBorrowedString,
    data: AsyncNatsBorrowedMessage,
) -> bool {
    let sender = unsafe { &*sender };

    let permit = sender.inner.sem.clone().try_acquire_owned();
    let Ok(permit) = permit else {
        return false;
    };

    let data_slice =
        unsafe { std::slice::from_raw_parts(data.0 as *const u8, data.1.try_into().unwrap()) };

    thread_local! {
        static BYTES: RefCell<BytesMut> = RefCell::new(bytes::BytesMut::with_capacity(65535));
    }
    let bytes = BYTES.with(|f| {
        let mut mbytes = f.borrow_mut();
        mbytes.extend_from_slice(data_slice);
        mbytes.split_to(data_slice.len()).freeze()
    });
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
    sender: *const AsyncNatsNamedSender,
    topic: AsyncNatsBorrowedString,
    data: AsyncNatsBorrowedMessage,
) {
    let sender = unsafe { &*sender };
    let permit = sender.inner.sem.clone().try_acquire_owned();
    let permit = match permit {
        Ok(x) => Some(x),
        Err(_) => None, // send without a permit
    };

    let data_slice =
        unsafe { std::slice::from_raw_parts(data.0 as *const u8, data.1.try_into().unwrap()) };

    thread_local! {
        static BYTES: RefCell<BytesMut> = RefCell::new(bytes::BytesMut::with_capacity(65535));
    }
    let bytes = BYTES.with(|f| {
        let mut mbytes = f.borrow_mut();
        mbytes.extend_from_slice(data_slice);
        mbytes.split_to(data_slice.len()).freeze()
    });
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
