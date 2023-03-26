use crate::api::{BorrowedString, LossyConvert};

pub struct TokioRuntime {
    pub(crate) runtime: tokio::runtime::Runtime,
}

impl TokioRuntime {
    pub fn new(cfg: &TokioRuntimeConfig) -> Self {
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
    cfg: *const TokioRuntimeConfig,
) -> *mut TokioRuntime {
    let cfg = unsafe { &*cfg };
    let tr = Box::new(TokioRuntime::new(cfg));
    Box::into_raw(tr)
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_delete(runtime: *mut TokioRuntime) {
    unsafe {
        drop(Box::from_raw(runtime));
    }
}

pub struct TokioRuntimeConfig {
    thread_name: String,
    thread_count: usize,
}

impl Default for TokioRuntimeConfig {
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
pub extern "C" fn async_nats_tokio_runtime_config_new() -> *mut TokioRuntimeConfig {
    let cfg = Box::new(TokioRuntimeConfig::default());
    Box::into_raw(cfg)
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_delete(cfg: *mut TokioRuntimeConfig) {
    unsafe {
        drop(Box::from_raw(cfg));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_thread_name(
    cfg: *mut TokioRuntimeConfig,
    thread_name: BorrowedString,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.thread_name = thread_name.lossy_convert();
}

#[no_mangle]
pub extern "C" fn async_nats_tokio_runtime_config_thread_count(
    cfg: *mut TokioRuntimeConfig,
    thread_count: u32,
) {
    let cfg = unsafe { &mut *cfg };
    cfg.thread_count = thread_count as usize;
}
