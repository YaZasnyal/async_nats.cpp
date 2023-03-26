use crate::api::Optional;

#[no_mangle]
pub extern "C" fn async_nats_io_error_delete(e: *mut std::io::Error) {
    unsafe {
        drop(Box::from_raw(e));
    }
}

#[no_mangle]
pub extern "C" fn async_nats_io_error_system_code(e: *const std::io::Error) -> Optional<i32> {
    let e = unsafe {&*e};
    let Some(e) = e.clone().raw_os_error() else {
        return Optional::none();
    };

    Optional::some(e)
}

#[no_mangle]
pub extern "C" fn async_nats_io_error_description(e: *const std::io::Error) -> *mut str {
    let e = unsafe {&*e};
    let bstr = e.clone().to_string().into_boxed_str();
    Box::into_raw(bstr)
}
