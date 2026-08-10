fn main() {
    cc::Build::new()
        .file("interflop_prism.c")
        .include(".")
        .include("../../../interflop-stdlib")
        .compile("interflop_prism");

    println!("cargo:rerun-if-changed=interflop_prism.c");
}
