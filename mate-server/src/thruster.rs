use std::{sync::Arc, time::Duration};

use axum::{Json, Router};
use serde::{Deserialize, Serialize};
use tokio::{sync::Mutex, time::Instant};

#[derive(Debug, Deserialize, Serialize)]
pub struct Thruster {
    pub top_left: f64,
    pub top_right: f64,
    pub bottom_left: f64,
    pub bottom_right: f64,
    pub side_left: f64,
    pub side_right: f64,
}

pub fn new() -> Router {
    tracing::info!("Connecting to microcontroller on /dev/tty.usbmodem11");
    let serial_ref = Arc::new(Mutex::new(
        tokio_serial::new("/dev/tty.usbmodem11", 112500)
            .open()
            .unwrap(),
    ));

    Router::new().route(
        "/",
        axum::routing::post(|Json(req): Json<Thruster>| async move {
            let mut buf: [u8; 48] = [0; 48];
            postcard::to_slice(&req, &mut buf).unwrap();

            let mut serial = serial_ref.lock().await;

            while serial.write(&[42]).unwrap() == 0 {}
            serial.write_all(&buf).unwrap();
            serial.flush().unwrap();

            let send_time = Instant::now();
            let mut recv_buf = [0; 48];
            let mut bytes_read = 0;
            while send_time.elapsed() < Duration::from_millis(500) {
                if let Ok(n) = serial.read(&mut recv_buf[..bytes_read]) {
                    bytes_read += n;
                }
            }

            let received_thruster_data: Thruster = postcard::from_bytes(&recv_buf).unwrap();
            Json(received_thruster_data)
        }),
    )
}
