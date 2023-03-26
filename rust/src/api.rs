/// This file contains common api structs
use std::ffi::{c_char, c_ulonglong, CStr};

pub trait LossyConvert {
    fn lossy_convert(&self) -> String;
}

impl LossyConvert for CStr {
    fn lossy_convert(&self) -> String {
        String::from_utf8_lossy(self.to_bytes()).to_string()
    }
}

/// BorrowedString is a C-string with lifetime limited to a specific function call
pub type BorrowedString = *const c_char;

impl LossyConvert for BorrowedString {
    fn lossy_convert(&self) -> String {
        unsafe { CStr::from_ptr(*self) }.lossy_convert()
    }
}

// OwnedString is a C-string whose lifetime is passed between the API bondary
pub type OwnedString = *mut c_char;

// AsyncString is a C-string which must be valid untill a callback is called  
pub type AsyncString = *mut c_char;
impl LossyConvert for AsyncString {
    fn lossy_convert(&self) -> String {
        unsafe { CStr::from_ptr(*self) }.lossy_convert()
    }
}

#[no_mangle]
pub extern "C" fn async_nats_owned_string_delete(s: OwnedString) {
    unsafe {
        drop(Box::from_raw(s));
    }
}

/// BorrowedMessage represents a byte stream with a lifetime limited to a 
/// specific function call.
#[repr(C)]
#[derive(Debug)]
pub struct BorrowedMessage(pub *const c_char, pub c_ulonglong);

/// AsyncMessage represents a byte stream with a lifetime limited to a
/// specific async call and should be valid until a callback is called
pub type AsyncMessage = BorrowedMessage;
unsafe impl Send for AsyncMessage {}

#[repr(C)]
#[derive(Debug)]
pub struct Slice {
    pub data: *const u8,
    pub size: u64,
}

impl Default for Slice {
    fn default() -> Self {
        Self {
            data: std::ptr::null(),
            size: Default::default(),
        }
    }
}

#[repr(C)]
pub struct Optional<T> {
    has_value: bool,
    value: T,
}

impl<T: Default> Optional<T> {
    pub fn some(val: T) -> Self {
        Self {
            has_value: true,
            value: val,
        }
    }

    pub fn none() -> Self {
        Self {
            has_value: false,
            value: T::default(),
        }
    }
}
