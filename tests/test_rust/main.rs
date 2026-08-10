fn main() {
    let mut a: f64 = 0.0;
    for _ in 0..100 {
        a += 0.1;
    }
    println!("a = {:.17}", a);
}
