use std::{thread, time::Duration};
use game_of_life::Universe;

fn main() {
    let mut u = Universe::random(120, 60, 12345);
    for _ in 0..200 {
        print!("\x1B[2J\x1B[H"); // clear screen and move cursor home
        println!("{}", u.as_string());
        u.tick();
        thread::sleep(Duration::from_millis(150));
    }
}
