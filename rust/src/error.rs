use crate::api::{AsyncNatsOwnedString, Optional};

pub type AsyncNatsIoError = i32;
pub type OsError = i32;

pub fn io_error_convert(e: std::io::Error) -> AsyncNatsIoError {
    e.kind() as i32
}

pub fn int_to_error(e: i32) -> std::io::ErrorKind {
    match e {
        x if x == std::io::ErrorKind::NotFound as i32 => std::io::ErrorKind::NotFound,
        x if x == std::io::ErrorKind::PermissionDenied as i32 => {
            std::io::ErrorKind::PermissionDenied
        }
        x if x == std::io::ErrorKind::ConnectionRefused as i32 => {
            std::io::ErrorKind::ConnectionRefused
        }
        x if x == std::io::ErrorKind::ConnectionReset as i32 => std::io::ErrorKind::ConnectionReset,
        // x if x == std::io::ErrorKind::HostUnreachable as i32 => std::io::ErrorKind::HostUnreachable,
        // x if x == std::io::ErrorKind::NetworkUnreachable as i32 => std::io::ErrorKind::NetworkUnreachable,
        x if x == std::io::ErrorKind::ConnectionAborted as i32 => {
            std::io::ErrorKind::ConnectionAborted
        }
        x if x == std::io::ErrorKind::NotConnected as i32 => std::io::ErrorKind::NotConnected,
        x if x == std::io::ErrorKind::AddrInUse as i32 => std::io::ErrorKind::AddrInUse,
        x if x == std::io::ErrorKind::AddrNotAvailable as i32 => {
            std::io::ErrorKind::AddrNotAvailable
        }
        // x if x == std::io::ErrorKind::NetworkDown as i32 => std::io::ErrorKind::NetworkDown,
        x if x == std::io::ErrorKind::BrokenPipe as i32 => std::io::ErrorKind::BrokenPipe,
        x if x == std::io::ErrorKind::AlreadyExists as i32 => std::io::ErrorKind::AlreadyExists,
        x if x == std::io::ErrorKind::WouldBlock as i32 => std::io::ErrorKind::WouldBlock,
        // x if x == std::io::ErrorKind::NotADirectory as i32 => std::io::ErrorKind::NotADirectory,
        // x if x == std::io::ErrorKind::IsADirectory as i32 => std::io::ErrorKind::IsADirectory,
        // x if x == std::io::ErrorKind::DirectoryNotEmpty as i32 => std::io::ErrorKind::DirectoryNotEmpty,
        // x if x == std::io::ErrorKind::ReadOnlyFilesystem as i32 => std::io::ErrorKind::ReadOnlyFilesystem,
        // x if x == std::io::ErrorKind::FilesystemLoop as i32 => std::io::ErrorKind::FilesystemLoop,
        // x if x == std::io::ErrorKind::StaleNetworkFileHandle as i32 => std::io::ErrorKind::StaleNetworkFileHandle,
        x if x == std::io::ErrorKind::InvalidInput as i32 => std::io::ErrorKind::InvalidInput,
        x if x == std::io::ErrorKind::InvalidData as i32 => std::io::ErrorKind::InvalidData,
        x if x == std::io::ErrorKind::TimedOut as i32 => std::io::ErrorKind::TimedOut,
        x if x == std::io::ErrorKind::WriteZero as i32 => std::io::ErrorKind::WriteZero,
        // x if x == std::io::ErrorKind::StorageFull as i32 => std::io::ErrorKind::StorageFull,
        // x if x == std::io::ErrorKind::NotSeekable as i32 => std::io::ErrorKind::NotSeekable,
        // x if x == std::io::ErrorKind::FilesystemQuotaExceeded as i32 => std::io::ErrorKind::FilesystemQuotaExceeded,
        // x if x == std::io::ErrorKind::FileTooLarge as i32 => std::io::ErrorKind::FileTooLarge,
        // x if x == std::io::ErrorKind::ResourceBusy as i32 => std::io::ErrorKind::ResourceBusy,
        // x if x == std::io::ErrorKind::ExecutableFileBusy as i32 => std::io::ErrorKind::ExecutableFileBusy,
        // x if x == std::io::ErrorKind::Deadlock as i32 => std::io::ErrorKind::Deadlock,
        // x if x == std::io::ErrorKind::CrossesDevices as i32 => std::io::ErrorKind::CrossesDevices,
        // x if x == std::io::ErrorKind::TooManyLinks as i32 => std::io::ErrorKind::TooManyLinks,
        // x if x == std::io::ErrorKind::InvalidFilename as i32 => std::io::ErrorKind::InvalidFilename,
        // x if x == std::io::ErrorKind::ArgumentListTooLong as i32 => std::io::ErrorKind::ArgumentListTooLong,
        // x if x == std::io::ErrorKind::Uncategorized as i32 => std::io::ErrorKind::Uncategorized,
        _ => std::io::ErrorKind::Other,
    }
}

#[no_mangle]
pub extern "C" fn async_nats_io_error_system_code(e: AsyncNatsIoError) -> Optional<OsError> {
    let kind = int_to_error(e);
    let Some(e) = std::io::Error::from(kind).raw_os_error() else {
        return Optional::none();
    };
    Optional::some(e)
}

#[no_mangle]
pub extern "C" fn async_nats_io_error_description(e: AsyncNatsIoError) -> AsyncNatsOwnedString {
    let kind = int_to_error(e);
    let e = std::io::Error::from(kind);
    let err = std::ffi::CString::new(e.to_string().as_bytes())
        .expect("Unable to convert error into CString");
    std::ffi::CString::into_raw(err)
}
