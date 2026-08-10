fn main() {
    cc::Build::new()
        .file("iostream/logger_shim.c")
        .include(".")
        .compile("logger_shim");

    println!("cargo:rerun-if-changed=iostream/logger_shim.c");
}
