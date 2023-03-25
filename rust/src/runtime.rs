use crate::config::RuntimeConfig;
use async_nats::Client;
use core::slice;
use std::ffi::{c_char, c_ulonglong, c_void};
use std::sync::Arc;
use tokio::sync::mpsc::{channel, Sender};
use futures::StreamExt;

use crate::api::LossyConvert;

#[derive(Debug)]
enum Operation {
    Send {
        topic: String,
        data: bytes::Bytes,
        cb: Option<Callback>,
    },
    None(),
}

/// Contains io threads and server connection
pub struct NatsRuntime {
    pub cfg: RuntimeConfig,
    pub handle: tokio::runtime::Runtime,
    pub client: Option<Arc<Client>>,
    chan: Option<Arc<Sender<Operation>>>,
}

#[no_mangle]
pub extern "C" fn nats_runtime_new(cfg: *const RuntimeConfig) -> *mut NatsRuntime {
    let cfg = unsafe { &*cfg };

    let runtime = tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .worker_threads(4)
        .thread_name("tokio")
        .build()
        .unwrap();

    let runtime = Box::new(NatsRuntime {
        cfg: cfg.clone(),
        handle: runtime,
        client: Default::default(),
        chan: Default::default(),
    });

    Box::into_raw(runtime)
}

#[no_mangle]
pub extern "C" fn nats_runtime_free(runtime: *mut NatsRuntime) {
    if runtime.is_null() {
        panic!("runtime pointer is nullptr");
    }
    let rt = unsafe { &mut *runtime };

    let handle = rt.handle.handle();
    let client = rt.client.clone();
    handle.block_on(async {
        if let Some(client) = client {
            client.flush().await.expect("unable to flush remaining data");
        }

    });
    
    unsafe {
        drop(Box::from_raw(runtime));
    }
}

#[no_mangle]
pub extern "C" fn nats_runtime_connect(runtime: *mut NatsRuntime) {
    let rt = unsafe { &mut *runtime };
    let handle = rt.handle.handle();
    handle.block_on(async {
        let client = Arc::new(
            async_nats::connect(rt.cfg.endpoints.as_slice())
                .await
                .unwrap(),
        );
        rt.client = Some(client.clone());

        let (tx, rx) = channel(100000 as usize);
        rt.chan = Some(Arc::new(tx));

        handle.spawn(async move {
            let mut rx = rx;
            loop {
                let msg = rx.recv().await.unwrap();
                match msg {
                    Operation::Send { topic, data, cb } => {
                        client.publish(topic, data).await.unwrap();
                        if let Some(cb) = cb {
                            cb.0(cb.1);
                        }
                    }
                    Operation::None() => {},
                }
            }
        });
    });
}

#[no_mangle]
pub extern "C" fn nats_runtime_bench(runtime: *const NatsRuntime) {
    let rt = unsafe { &*runtime };

    let f = rt.handle.spawn(async {
        loop {
            // rt.chan.as_ref().unwrap().send(()).await.unwrap();
            rt.client
                .as_ref()
                .unwrap()
                .publish("test".into(), "Hello from rust".into())
                .await
                .unwrap();
        }
    });
    rt.handle.block_on(f).unwrap();
}

#[no_mangle]
pub extern "C" fn nats_runtime_bench_read(runtime: *const NatsRuntime) {
    let rt = unsafe { &*runtime };

    let f = rt.handle.spawn(async {
        let mut sub = rt.client.as_ref().unwrap().subscribe("test".into()).await.unwrap();
        loop {
            sub.next().await.unwrap();
        }
        // rt.client.as_ref().unwrap().flush().await.unwrap();
    });
    rt.handle.block_on(f).unwrap();
}

/// Topic is a null-terminated string
#[repr(C)]
#[derive(Debug)]
pub struct Topic(*const c_char);
unsafe impl Send for Topic {}
#[repr(C)]
#[derive(Debug)]
pub struct Message(*const c_char, c_ulonglong);
#[repr(C)]
#[derive(Debug)]
pub struct SMessage(*const c_char, c_ulonglong);
unsafe impl Send for Message {}
#[repr(C)]
#[derive(Debug, Clone)]
pub struct Callback(extern "C" fn(d: *const c_void), *const c_void);
unsafe impl Send for Callback {}

#[no_mangle]
pub extern "C" fn nats_runtime_try_send(
    runtime: *const NatsRuntime,
    topic: Topic,
    message: SMessage,
) -> bool {
    let rt = unsafe { &*runtime };

    let topic_str = topic.0.lossy_convert();
    let data_slice =
        unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
    let bytes = bytes::Bytes::from_static(data_slice);

    let res = rt.chan.as_ref().unwrap().try_send(Operation::Send {
        topic: topic_str,
        data: bytes.clone(),
        cb: None,
    });

    return res.is_ok();
}

#[no_mangle]
pub extern "C" fn nats_runtime_send(
    runtime: *const NatsRuntime,
    topic: Topic,
    message: SMessage,
    cb: Callback,
) {
    let rt = unsafe { &*runtime };
    let topic_str = topic.0.lossy_convert();
    let data_slice =
    unsafe { slice::from_raw_parts(message.0 as *const u8, message.1.try_into().unwrap()) };
    let bytes = bytes::Bytes::from_static(data_slice);

    rt.handle.spawn(async move {
        rt.chan.as_ref().unwrap().send(Operation::Send {
            topic: topic_str,
            data: bytes.clone(),
            cb: Some(cb),
        }).await.unwrap();
    });
}
