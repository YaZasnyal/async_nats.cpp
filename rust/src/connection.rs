use crate::tokio_runtime::TokioRuntime;

use async_nats::{Client, ConnectOptions, connect_with_options};
use std::sync::Arc;

#[derive(Clone)]
pub struct Connection {
    rt: tokio::runtime::Handle,
    client: Option<Client>,
}

impl Connection {
    pub fn new(h: tokio::runtime::Handle) -> Self {
        Self {
            rt: h.clone(),
            client: Default::default(),
        }
    }

    pub async fn connect(&mut self) -> Result<(), std::io::Error> {
        let co = ConnectOptions::default();
        // let res = connect_with_options("127".into(), co).await?;
        Ok(())
    }
}

#[no_mangle]
pub extern "C" fn async_nats_connection_new(rt: *const TokioRuntime) -> *mut Connection {
    let rt = unsafe { &*rt };
    let conn = Box::new(Connection::new(rt.handle()));
    Box::into_raw(conn)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_clone(conn: *const Connection) -> *mut Connection {
    let conn = unsafe { &*conn };
    let new_conn = Box::new(conn.clone());
    Box::into_raw(new_conn)
}

#[no_mangle]
pub extern "C" fn async_nats_connection_delete(conn: *mut Connection) {
    unsafe {
        drop(Box::from_raw(conn));
    }
}
