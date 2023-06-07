use std::ffi::c_ulonglong;

use crate::message::AsyncNatsMessage;
use crate::subscribtion::AsyncNatsSubscribtion;
use crossbeam::channel::{bounded, Receiver};

#[derive(Clone)]
pub struct AsyncNatsNamedReceiver {
    receiver: Receiver<async_nats::Message>,
}

impl AsyncNatsNamedReceiver {
    pub fn new(w: AsyncNatsSubscribtion, capacity: usize) -> Self {
        let rt = w.rt.clone();
        let (tx, rx) = bounded(capacity);
        rt.spawn(async move {
            let mut w = w;
            loop {
                let Some(msg) = w.inner.pop().await else {
                    return;
                };
                tx.try_send(msg).ok();
            }
        });

        Self { receiver: rx }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_new(
    s: *mut AsyncNatsSubscribtion,
    capacity: c_ulonglong,
) -> *mut AsyncNatsNamedReceiver {
    let sub = unsafe { Box::from_raw(s) };
    let recv = Box::new(AsyncNatsNamedReceiver::new(*sub, capacity as usize));
    Box::into_raw(recv)
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_clone(
    recv: *const AsyncNatsNamedReceiver,
) -> *mut AsyncNatsNamedReceiver {
    let receiver = unsafe { &*recv };
    Box::into_raw(Box::new(receiver.clone()))
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_delete(recv: *mut AsyncNatsNamedReceiver) {
    unsafe {
        drop(Box::from_raw(recv));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_try_recv(
    s: *const AsyncNatsNamedReceiver,
) -> *mut AsyncNatsMessage {
    let receiver = unsafe { &*s };
    let Ok(msg) = receiver.receiver.try_recv() else {
        return std::ptr::null_mut();
    };
    Box::into_raw(Box::new(msg.into()))
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_recv(
    s: *const AsyncNatsNamedReceiver,
) -> *mut AsyncNatsMessage {
    let receiver = unsafe { &*s };
    let Ok(msg) = receiver.receiver.recv() else {
        return std::ptr::null_mut();
    };
    Box::into_raw(Box::new(msg.into()))
}
