use crate::message::AsyncNatsMessage;
use async_nats::{Message, Subscriber};
use futures::{FutureExt, StreamExt};
use std::ffi::c_void;

pub struct Subscribtion {
    sub: Subscriber,
}

impl Subscribtion {
    pub async fn pop(&mut self) -> Option<Message> {
        loop {
            futures::select_biased! {
                msg = self.sub.next().fuse() => {
                    return msg;
                },
                // _ = (&mut self.shutdown_receiver).fuse() => {
                //     self.chan.unsubscribe().await.expect("Unable to unsubscribe from channel");
                // },
            };
        }
    }
}

pub struct AsyncNatsSubscribtion {
    pub(crate) rt: tokio::runtime::Handle,
    pub(crate) inner: Subscribtion,
}

impl AsyncNatsSubscribtion {
    pub fn new(rt: tokio::runtime::Handle, sub: Subscriber) -> Self {
        Self {
            rt,
            inner: Subscribtion { sub },
        }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_delete(s: *mut AsyncNatsSubscribtion) {
    let mut s = unsafe {
        Box::from_raw(s)
    };
    
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

        let boxed_msg = Box::new(AsyncNatsMessage(msg));
        cb.0(Box::into_raw(boxed_msg), cb.1);
    });
}
