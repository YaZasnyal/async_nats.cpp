use core::ffi::CStr;
use std::{os::raw::c_char, str::FromStr};

use async_nats::ServerAddr;

#[derive(Debug, Clone)]
pub struct RuntimeConfig {
    /// nats server connection info
    pub endpoints: Vec<ServerAddr>,
}

#[no_mangle]
pub extern "C" fn nats_runtime_config_new() -> *mut RuntimeConfig {
    Box::into_raw(Box::new(RuntimeConfig {
        endpoints: Default::default(),
    }))
}

#[no_mangle]
pub extern "C" fn nats_runtime_config_free(cfg: *mut RuntimeConfig) {
    if cfg.is_null() {
        panic!("cfg pointer is nullptr");
    }
    unsafe {
        drop(Box::from_raw(cfg));
    }
}

#[no_mangle]
pub extern "C" fn nats_runtime_config_set_endpoint(
    cfg: *mut RuntimeConfig,
    endpoint: *const c_char,
) {
    if cfg.is_null() {
        panic!("cfg pointer is nullptr");
    }
    let cfg = unsafe { &mut *cfg };
    let cstr = unsafe { CStr::from_ptr(endpoint) };
    let ep = String::from_utf8_lossy(cstr.to_bytes()).to_string();
    cfg.endpoints.push(ServerAddr::from_str(ep.as_str()).unwrap());
    // println!("RuntimeConfig: {:#?}", cfg);
}
