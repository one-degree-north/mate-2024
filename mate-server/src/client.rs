#[cfg(debug_assertions)]
pub fn new() -> axum::Router {
    tokio::spawn(async {
        use std::process::Stdio;

        tracing::info!("Running in debug mode, serving client from yarn dev");
        let mut process = std::process::Command::new("yarn")
            .args(["dev", "--strictPort", "--port", "5173", "--host"])
            .stdout(Stdio::null())
            .current_dir("../mate-client")
            .spawn()
            .unwrap();

        std::thread::sleep(std::time::Duration::from_secs(2));

        assert!(
            process.try_wait().unwrap().is_none(),
            "yarn dev failed, port 5173 may already be in use"
        );

        tracing::info!("Yarn dev server started on port 5173");
    });

    axum::Router::new().route(
        "/",
        axum::routing::get(|| async { axum::response::Redirect::to("http://localhost:5173") }),
    )
}

#[cfg(not(debug_assertions))]
pub fn new() -> axum::Router {
    static CLIENT_DIR: include_dir::DirEntry =
        include_dir::DirEntry::Dir(include_dir::include_dir!("../mate-client/dist"));

    let mut router = axum::Router::new().route(
        "/",
        axum::routing::get(|| async {
            axum::response::Html(
                CLIENT_DIR
                    .as_dir()
                    .unwrap()
                    .get_file("index.html")
                    .unwrap()
                    .contents(),
            )
        }),
    );

    let mut queue = std::collections::VecDeque::from([&CLIENT_DIR]);

    while let Some(source) = queue.pop_front() {
        match source {
            include_dir::DirEntry::File(file) => {
                tracing::info!("Adding route for {}", file.path().to_str().unwrap());
                router = router.route(
                    &format!("/{}", file.path().to_str().unwrap()),
                    axum::routing::get(|| async {
                        (
                            [(
                                axum::http::header::CONTENT_TYPE,
                                mime_guess::from_path(file.path())
                                    .first_or_text_plain()
                                    .to_string(),
                            )],
                            file.contents(),
                        )
                    }),
                )
            }
            include_dir::DirEntry::Dir(dir) => queue.extend(dir.entries().iter()),
        }
    }

    router
}
