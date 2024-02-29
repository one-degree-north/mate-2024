use std::convert::Infallible;

use axum::{
    extract::{
        ws::{Message, WebSocket},
        WebSocketUpgrade,
    },
    routing::get,
    Router,
};
use opencv::{
    core::{Mat, Vector},
    prelude::*,
    videoio::VideoCapture,
};

pub fn new() -> Router {
    let (tx, mut rx) = tokio::sync::watch::channel::<Result<Message, Infallible>>(Ok(
        Message::Text("".to_string()),
    ));

    tokio::spawn(async move {
        tracing::info!("Starting camera capture with camera at index 0 (CAP_ANY)");
        let mut video_capture = VideoCapture::new(0, opencv::videoio::CAP_V4L).unwrap();
        let mut frame = Mat::default();
        let mut interval = tokio::time::interval(std::time::Duration::from_millis(32)); // 30 fps

        loop {
            if video_capture.read(&mut frame).unwrap() {
                let mut buf = Vector::<u8>::new();
                opencv::imgcodecs::imencode(".jpg", &frame, &mut buf, &opencv::core::Vector::new())
                    .unwrap();

                tx.send(Ok(Message::Binary(buf.to_vec()))).unwrap();

                interval.tick().await;
            }
        }
    });

    Router::new().route(
        "/",
        get(|ws: WebSocketUpgrade| async {
            ws.on_upgrade(|mut socket: WebSocket| async move {
                loop {
                    let msg = rx.borrow_and_update().to_owned().unwrap();
                    socket.send(msg).await.unwrap();
                }
            })
        }),
    )
}
