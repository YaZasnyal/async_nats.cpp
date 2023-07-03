/// This file contains common api structs
use std::ffi::{c_char, c_ulonglong, c_void, CStr};

pub trait LossyConvert {
    fn lossy_convert(&self) -> String;
}

impl LossyConvert for CStr {
    fn lossy_convert(&self) -> String {
        String::from_utf8_lossy(self.to_bytes()).to_string()
    }
}

/// BorrowedString is a C-string with lifetime limited to a specific function call
/// or while object that produced this string is valid
pub type AsyncNatsBorrowedString = *const c_char;

impl LossyConvert for *const c_char {
    fn lossy_convert(&self) -> String {
        unsafe { CStr::from_ptr(*self) }.lossy_convert()
    }
}

// OwnedString is a C-string whose lifetime is passed between the API bondary
pub type AsyncNatsOwnedString = *mut c_char;

/// Convert String to AsyncNatsOwnedString
///
/// This function panics if conversion cannot be made
pub(crate) fn string_to_owned_string(s: String) -> AsyncNatsOwnedString {
    let err = std::ffi::CString::new(s.as_bytes()).expect("Unable to convert error into CString");
    std::ffi::CString::into_raw(err)
}

// AsyncString is a C-string which must be valid untill a callback is called
pub type AsyncNatsAsyncString = AsyncNatsBorrowedString;

#[no_mangle]
pub extern "C" fn async_nats_owned_string_delete(s: AsyncNatsOwnedString) {
    unsafe {
        drop(Box::from_raw(s));
    }
}

/// BorrowedMessage represents a byte stream with a lifetime limited to a
/// specific function call.
#[repr(C)]
#[derive(Debug)]
pub struct AsyncNatsBorrowedMessage(pub *const c_void, pub c_ulonglong);

/// AsyncMessage represents a byte stream with a lifetime limited to a
/// specific async call and should be valid until a callback is called
pub type AsyncNatsAsyncMessage = AsyncNatsBorrowedMessage;
unsafe impl Send for AsyncNatsAsyncMessage {}

#[repr(C)]
#[derive(Debug)]
pub struct AsyncNatsSlice {
    pub data: *const c_void,
    pub size: u64,
}

impl AsyncNatsSlice {
    pub fn as_slice(&self) -> Option<&[u8]> {
        if self.data == std::ptr::null() {
            return None;
        }

        Some(unsafe {
            core::slice::from_raw_parts::<u8>(self.data as *const u8, self.size as usize)
        })
    }

    pub fn as_str(&self) -> Option<&str> {
        let Some(data) = self.as_slice() else {
            return None;
        };

        Some(unsafe { core::str::from_utf8_unchecked(data) })
    }
}

impl Default for AsyncNatsSlice {
    fn default() -> Self {
        Self {
            data: std::ptr::null(),
            size: Default::default(),
        }
    }
}

impl LossyConvert for AsyncNatsSlice {
    fn lossy_convert(&self) -> String {
        String::from_utf8_lossy(unsafe {
            core::slice::from_raw_parts::<u8>(self.data as *const u8, self.size as usize)
        })
        .into()
    }
}
