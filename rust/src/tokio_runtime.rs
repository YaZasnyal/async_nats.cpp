use crate::api::{AsyncNatsBorrowedString, LossyConvert};

pub struct AsyncNatsTokioRuntime {
    pub(crate) runtime: tokio::runtime::Runtime,
}

impl AsyncNatsTokioRuntime {
    pub fn new(cfg: &AsyncNatsTokioRuntimeConfig) -> Self {
        let runtime = tokio::runtime::Builder::new_multi_thread()
            .enable_all()
            .worker_threads(cfg.thread_count)
            .thread_name(cfg.thread_name.as_str())
            .build()
            .expect("Unable to create tokio runtime");

        Self { runtime }
    }

    pub fn handle(&self) -> tokio::runtime::Handle {
        self.runtime.handle().clone()
    }
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_new(
    cfg: *const AsyncNatsTokioRuntimeConfig,
) -> *mut AsyncNatsTokioRuntime {
    let cfg = unsafe { &*cfg };
    let tr = Box::new(AsyncNatsTokioRuntime::new(cfg));
    Box::into_raw(tr)
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_delete(runtime: *mut AsyncNatsTokioRuntime) {
    unsafe {
        drop(Box::from_raw(runtime));
    }
}

pub struct AsyncNatsTokioRuntimeConfig {
    thread_name: String,
    thread_count: usize,
}

impl Default for AsyncNatsTokioRuntimeConfig {
    fn default() -> Self {
        Self {
            thread_name: "tokio_runtime".into(),
            thread_count: {
                match std::thread::available_parallelism() {
                    Ok(x) => x.into(),
                    Err(_) => 1,
                }
            },
        }
    }
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_new() -> *mut AsyncNatsTokioRuntimeConfig {
    let cfg = Box::new(AsyncNatsTokioRuntimeConfig::default());
    Box::into_raw(cfg)
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_delete(cfg: *mut AsyncNatsTokioRuntimeConfig) {
    unsafe {
        drop(Box::from_raw(cfg));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_thread_name(
    cfg: *mut AsyncNatsTokioRuntimeConfig,
    thread_name: AsyncNatsBorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.thread_name = thread_name.lossy_convert();
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_thread_count(
    cfg: *mut AsyncNatsTokioRuntimeConfig,
    thread_count: u32,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.thread_count = thread_count as usize;
}
