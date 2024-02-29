use std::convert::Infallible;

use axum::{
    response::{
        sse::{Event, KeepAlive},
        Sse,
    },
    routing::get,
    Router,
};
use bno055::{
    mint::{EulerAngles, Quaternion, Vector3},
    BNO055OperationMode,
};
use linux_embedded_hal::Delay;
use serde::{Deserialize, Serialize};
use tokio_stream::wrappers::WatchStream;

#[derive(Debug, Deserialize, Serialize)]
pub struct BNOData {
    pub euler: EulerAngles<f32, ()>,
    pub quaternion: Quaternion<f32>,
    pub linear_acceleration: Vector3<f32>,
    pub magnetometer: Vector3<f32>,
    pub gyroscope: Vector3<f32>,
    pub acceleration: Vector3<f32>,
    pub gravity: Vector3<f32>,
}

pub fn new() -> Router {
    let (tx, rx) = tokio::sync::watch::channel::<Result<Event, Infallible>>(Ok(Event::default()));

    tokio::spawn(async move {
        let mut i2c = linux_embedded_hal::I2cdev::new("/dev/i2c-1").unwrap();

        let mut delay = Delay {};

        let mut bno = bno055::Bno055::new(&mut i2c).with_alternative_address();
        bno.init(&mut delay).unwrap();
        bno.set_mode(BNO055OperationMode::NDOF, &mut delay).unwrap();

        tracing::info!("Starting BNO calibration");
        tracing::info!(
            "Current calibration status {:?}\r",
            bno.get_calibration_status().unwrap()
        );

        // while !bno.is_fully_calibrated().unwrap() {
        //     tracing::info!(
        //         "calibration status: {:?}\r",
        //         bno.get_calibration_status().unwrap()
        //     );
        //     std::thread::sleep(std::time::Duration::from_millis(1000));
        // }

        // let calib = bno.calibration_profile(&mut delay).unwrap();
        // bno.set_calibration_profile(calib, &mut delay).unwrap();

        // tracing::info!("BNO calibrated");

        let mut interval = tokio::time::interval(std::time::Duration::from_millis(100));

        loop {
            tx.send(Ok(Event::default()
                .json_data(BNOData {
                    euler: bno.euler_angles().unwrap(),
                    quaternion: bno.quaternion().unwrap(),
                    linear_acceleration: bno.linear_acceleration().unwrap(),
                    magnetometer: bno.mag_data().unwrap(),
                    acceleration: bno.accel_data().unwrap(),
                    gyroscope: bno.gyro_data().unwrap(),
                    gravity: bno.gravity().unwrap(),
                })
                .unwrap()))
                .unwrap();

            interval.tick().await;
        }
    });

    Router::new().route(
        "/",
        get(|| async { Sse::new(WatchStream::new(rx)).keep_alive(KeepAlive::default()) }),
    )
}
