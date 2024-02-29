use std::sync::Arc;

use axum::{
    response::{
        sse::{Event, KeepAlive},
        Sse,
    },
    routing::get,
    Router,
};
use tokio::net::TcpListener;
use tokio_stream::wrappers::BroadcastStream;
use tower_http::{cors::CorsLayer, trace::TraceLayer};

mod camera;
mod client;
mod data;
mod thruster;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();
    tracing::info!("Starting server");

    // let (sender, receiver) = tokio::sync::broadcast::channel::<Event>(16);
    // let rx = Arc::new(receiver);

    let app = Router::new()
        .nest("/", client::new())
        .nest("/camera", camera::new())
        // .nest("/thruster", thruster::new())
        // .route(
        //     "/data",
        //     get(|| async move {
        //         Sse::new(BroadcastStream::new(rx.resubscribe())).keep_alive(KeepAlive::default())
        //     }),
        // )
        .nest("/data", data::new())
        .layer(TraceLayer::new_for_http())
        .layer(CorsLayer::permissive());

    let listener = TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}
