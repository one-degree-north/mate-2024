use std::process::Command;

#[cfg(not(debug_assertions))]
fn main() {
    println!("cargo:rerun-if-changed=../mate-client/dist");
    println!("cargo:rerun-if-changed=../mate-client/node_modules");

    assert!(
        Command::new("yarn")
            .current_dir("../mate-client")
            .output()
            .unwrap()
            .status
            .success(),
        "yarn install failed"
    );

    assert!(
        Command::new("yarn")
            .args(["build", "--mode", "production"])
            .current_dir("../mate-client")
            .output()
            .unwrap()
            .status
            .success(),
        "yarn build failed"
    );
}

#[cfg(debug_assertions)]
fn main() {
    println!("cargo:rerun-if-changed=../mate-client/node_modules");

    assert!(
        Command::new("yarn")
            .current_dir("../mate-client")
            .output()
            .unwrap()
            .status
            .success(),
        "yarn install failed"
    );
}
