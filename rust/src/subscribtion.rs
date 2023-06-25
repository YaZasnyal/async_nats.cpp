use crate::message::AsyncNatsMessage;
use async_nats::{Message, Subscriber};
use futures::{FutureExt, StreamExt};
use std::ffi::c_void;

pub struct Subscribtion {
    sub: Subscriber,
    sd_receiver: tokio::sync::mpsc::Receiver<()>,
}

impl Subscribtion {
    pub async fn pop(&mut self) -> Option<Message> {
        loop {
            futures::select_biased! {
                msg = self.sub.next().fuse() => {
                    return msg;
                },
                _ = self.sd_receiver.recv().fuse() => {
                    self.sub.unsubscribe().await.expect("Unable to unsubscribe from channel");
                },
            };
        }
    }
}

pub struct AsyncNatsSubscribtion {
    pub(crate) rt: tokio::runtime::Handle,
    pub(crate) inner: Subscribtion,
    // sd_sender: std::sync::Arc<tokio::sync::oneshot::Sender<()>>,/
    sd_sender: tokio::sync::mpsc::Sender<()>,
}

impl AsyncNatsSubscribtion {
    pub fn new(rt: tokio::runtime::Handle, sub: Subscriber) -> Self {
        let (tx, rx) = tokio::sync::mpsc::channel(1);
        Self {
            rt,
            inner: Subscribtion {
                sub,
                sd_receiver: rx,
            },
            sd_sender: tx,
        }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_delete(s: *mut AsyncNatsSubscribtion) {
    let mut s = unsafe { Box::from_raw(s) };

    let rt = s.rt.clone();
    rt.spawn(async move {
        s.inner.sub.unsubscribe().await.ok();
        drop(s);
    });
}

#[repr(C)]
pub struct AsyncNatsReceiveCallback(
    extern "C" fn(m: *mut AsyncNatsMessage, c: *mut c_void),
    *mut c_void,
);
unsafe impl Send for AsyncNatsReceiveCallback {}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_receive_async(
    s: *mut AsyncNatsSubscribtion,
    cb: AsyncNatsReceiveCallback,
) {
    let s = unsafe { &mut *s };
    s.rt.spawn(async {
        let cb = cb;
        let res = s.inner.pop().await;
        let Some(msg) = res else {
            cb.0(std::ptr::null_mut(), cb.1);
            return;
        };

        let boxed_msg: Box<AsyncNatsMessage> = Box::new(msg.into());
        cb.0(Box::into_raw(boxed_msg), cb.1);
    });
}

pub struct AsyncNatsSubscribtionCancellationToken {
    sd_sender: tokio::sync::mpsc::Sender<()>,
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_get_cancellation_token(
    s: *mut AsyncNatsSubscribtion,
) -> *mut AsyncNatsSubscribtionCancellationToken {
    let s = unsafe { &mut *s };
    let token = Box::new(AsyncNatsSubscribtionCancellationToken {
        sd_sender: s.sd_sender.clone(),
    });
    Box::into_raw(token)
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_cancellation_token_clone(
    c: *mut AsyncNatsSubscribtionCancellationToken,
) -> *mut AsyncNatsSubscribtionCancellationToken {
    let c = unsafe { &mut *c };
    let token = Box::new(AsyncNatsSubscribtionCancellationToken {
        sd_sender: c.sd_sender.clone(),
    });
    Box::into_raw(token)
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_cancellation_token_delete(
    c: *mut AsyncNatsSubscribtionCancellationToken,
) {
    let c = unsafe { Box::from_raw(c) };
    drop(c);
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_cancellation_token_cancel(
    c: *mut AsyncNatsSubscribtionCancellationToken,
) {
    let c = unsafe { &mut *c };
    c.sd_sender.try_send(()).ok();
}
