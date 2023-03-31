use async_nats::{Message, Subscriber};
use std::ffi::c_void;
use futures::{StreamExt, FutureExt};

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

pub struct SubscribtionWrapper {
    pub(crate) rt: tokio::runtime::Handle,
    pub(crate) inner: Subscribtion,
}

impl SubscribtionWrapper {
    pub fn new(rt: tokio::runtime::Handle, sub: Subscriber) -> Self {
        Self {
            rt,
            inner: Subscribtion {
                sub,
            },
        }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_delete(s: *mut SubscribtionWrapper) {
    unsafe {
        drop(Box::from_raw(s));
    }
}

#[repr(C)]
pub struct ReceiveCallback(extern "C" fn(m: *mut Message, c: *mut c_void), *mut c_void);
unsafe impl Send for ReceiveCallback {}

#[no_mangle]
pub extern "C" fn async_nats_subscribtion_receive_async(s: *mut SubscribtionWrapper, cb: ReceiveCallback) {
    let s = unsafe { &mut *s };
    s.rt.spawn(async {
        let cb = cb;
        let res = s.inner.pop().await;
        let Some(msg) = res else {
            cb.0(std::ptr::null_mut(), cb.1);
            return;
        };

        let boxed_msg = Box::new(msg);
        cb.0(Box::into_raw(boxed_msg), cb.1);
    });
}
