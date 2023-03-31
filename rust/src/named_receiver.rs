use std::ffi::c_ulonglong;

use crate::subscribtion::SubscribtionWrapper;
use async_nats::Message;
use crossbeam::channel::{bounded, Receiver};

pub struct NamedReceiver {
    receiver: Receiver<Message>,
}

impl NamedReceiver {
    pub fn new(w: SubscribtionWrapper, capacity: usize) -> Self {
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
    s: *mut SubscribtionWrapper,
    capacity: c_ulonglong,
) -> *mut NamedReceiver {
    let sub = unsafe { Box::from_raw(s) };
    let recv = Box::new(NamedReceiver::new(*sub, capacity as usize));
    Box::into_raw(recv)
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_delete(recv: *mut NamedReceiver) {
    unsafe {
        drop(Box::from_raw(recv));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_try_recv(s: *const NamedReceiver) -> *mut Message {
    let receiver = unsafe { &*s };
    let Ok(msg) = receiver.receiver.try_recv() else {
        return std::ptr::null_mut();
    };
    Box::into_raw(Box::new(msg))
}

#[no_mangle]
pub extern "C" fn async_nats_named_receiver_recv(s: *const NamedReceiver) -> *mut Message {
    let receiver = unsafe { &*s };
    let Ok(msg) = receiver.receiver.recv() else {
        return std::ptr::null_mut();
    };
    Box::into_raw(Box::new(msg))
}
