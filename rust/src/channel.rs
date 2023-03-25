use async_nats::Subscriber;
use async_nats::{Message};
use futures::{StreamExt, FutureExt};
use std::sync::Arc;
use std::sync::Mutex;
use tokio::sync::mpsc::{channel, error::TryRecvError, Receiver};
use tokio::sync::oneshot as os;
use std::ffi::c_void;

use crate::api::{BorrowedString, LossyConvert, NatsMessage};
use crate::runtime::NatsRuntime;

/// must implement Send
pub struct Channel {
    /// data channel
    chan: Subscriber,
    /// shutdown channel
    shutdown_sender: Arc<Mutex<Option<os::Sender<()>>>>,
    shutdown_receiver: os::Receiver<()>,
}

impl Channel {
    pub async fn new_with_size(rt: &NatsRuntime, topic: String) -> Channel {
        if rt.client.is_none() {
            panic!("No client is present!");
        }

        let client = rt.client.as_ref().unwrap();
        let sub = client
            .subscribe(topic)
            .await
            .expect("Unable to subscribe to topic");

        let (close_tx, close_rx) = os::channel::<()>();

        Self {
            chan: sub,
            shutdown_sender: Arc::new(Mutex::new(Some(close_tx))),
            shutdown_receiver: close_rx,
        }
    }

    // pop - blocking wait
    pub async fn pop(&mut self) -> Option<Message> {
        loop {
            futures::select_biased! {
                msg = self.chan.next().fuse() => {
                    return msg;
                },
                _ = (&mut self.shutdown_receiver).fuse() => {
                    self.chan.unsubscribe().await.expect("Unable to unsubscribe from channel");
                },
            };
        }
    }

    pub async fn close(&mut self) {
        let mut lock = self.shutdown_sender.lock().unwrap();
        if let Some(sd) = lock.take() {
            self.chan.unsubscribe().await.expect("Unable to unsubscribe");
        }
    }

    pub fn get_cancellation_token(&self) -> CancellationToken {
        CancellationToken {
            shutdown: self.shutdown_sender.clone(),
        }
    }
}

/// Wrapper for Channel that is returned to the caller
pub struct ChannelWrp {
    handle: tokio::runtime::Handle,
    chan: Channel,
}

#[no_mangle]
pub extern "C" fn nats_channel_create(
    rt: *const NatsRuntime,
    topic: BorrowedString
) -> *mut ChannelWrp {
    let rt = unsafe { &(*rt) };

    let chan = rt.handle.block_on(async {
        Channel::new_with_size(&rt, topic.lossy_convert()).await
    });

    let chan = Box::new(ChannelWrp {
        handle: rt.handle.handle().clone(),
        chan
    });
    Box::into_raw(chan)
}

#[no_mangle]
pub extern "C" fn nats_channel_delete(chan: *mut ChannelWrp) {
    if chan.is_null() {
        panic!("Channel is nullptr");
    }

    let chan = unsafe { &mut *chan };
    let handle = chan.handle.clone();
    handle.block_on(async move {
        let mut chan = unsafe {
            Box::from_raw(chan)
        };
        chan.chan.chan.unsubscribe().await.unwrap();
    });
}

#[no_mangle]
pub extern "C" fn nats_channel_recv(chan: *mut ChannelWrp) -> *mut NatsMessage {
    let chan = unsafe { &mut *chan };
    let msg = chan.handle.block_on(async { 
        chan.chan.pop().await 
    });
    let Some(msg) = msg else {
        return std::ptr::null_mut();
    };
    let boxed_msg = Box::new(msg);
    Box::into_raw(boxed_msg)
}

#[repr(C)]
pub struct ChannelCallback(extern "C" fn(m: *mut Message, c: *mut c_void), *mut c_void);
unsafe impl Send for ChannelCallback {}

#[no_mangle]
pub extern "C" fn nats_channel_recv_async(chan: *mut ChannelWrp, cb: ChannelCallback) {
    let chan = unsafe { &mut *chan };
    chan.handle.spawn(async {
        let cb = cb;
        let res = chan.chan.pop().await;
        let Some(msg) = res else {
            cb.0(std::ptr::null_mut(), cb.1);
            return;
        };
        
        let boxed_msg = Box::new(msg);
        cb.0(Box::into_raw(boxed_msg), cb.1);
    });
}


#[no_mangle]
pub extern "C" fn nats_channel_bench(chan: *mut ChannelWrp, count: u64) {
    let chan = unsafe { &mut *chan };
    chan.handle.block_on(async {
        for _ in 1..count {
          chan.chan.pop().await;
        }
    });
}



/// CancellationToken allows to close channel
pub struct CancellationToken {
    shutdown: Arc<Mutex<Option<os::Sender<()>>>>,
}

impl CancellationToken {
    pub fn close(&self) {
        let mut lock = self.shutdown.lock().unwrap();
        if let Some(sd) = lock.take() {
            sd.send(()).ok();
        }
    }
}

// get cancellation_token
// free cancellation_token
// cancel cancellation_token
