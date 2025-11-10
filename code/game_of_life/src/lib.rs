//! Core Game of Life logic. No I/O here. Suitable for unit tests and reuse in wasm.

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Universe {
    width: u32,
    height: u32,
    cells: Vec<u8>, // 0 = dead, 1 = alive
}

impl Universe {
    pub fn new(width: u32, height: u32) -> Self {
        let size = (width * height) as usize;
        Self {
            width,
            height,
            cells: vec![0; size],
        }
    }

    pub fn width(&self) -> u32 { self.width }
    pub fn height(&self) -> u32 { self.height }

    fn idx(&self, row: u32, col: u32) -> usize {
        (row * self.width + col) as usize
    }

    pub fn get(&self, row: u32, col: u32) -> u8 {
        self.cells[self.idx(row % self.height, col % self.width)]
    }

    pub fn set(&mut self, row: u32, col: u32, alive: bool) {
        let i = self.idx(row % self.height, col % self.width);
        self.cells[i] = if alive { 1 } else { 0 };
    }

    pub fn toggle(&mut self, row: u32, col: u32) {
        let i = self.idx(row % self.height, col % self.width);
        self.cells[i] ^= 1;
    }

    fn live_neighbor_count(&self, row: u32, col: u32) -> u8 {
        let mut count = 0u8;
        for delta_row in [self.height - 1, 0, 1] {
            for delta_col in [self.width - 1, 0, 1] {
                if delta_row == 0 && delta_col == 0 { continue; }
                let nrow = (row + delta_row) % self.height;
                let ncol = (col + delta_col) % self.width;
                count += self.get(nrow, ncol);
            }
        }
        count
    }

    pub fn tick(&mut self) {
        let mut next = self.cells.clone();
        for row in 0..self.height {
            for col in 0..self.width {
                let idx = self.idx(row, col);
                let cell = self.cells[idx];
                let live_neighbors = self.live_neighbor_count(row, col);
                let next_cell = match (cell, live_neighbors) {
                    (1, x) if x < 2 => 0,                // Underpopulation
                    (1, 2) | (1, 3) => 1,               // Lives on
                    (1, x) if x > 3 => 0,               // Overpopulation
                    (0, 3) => 1,                        // Reproduction
                    (current, _) => current,            // Stays the same
                };
                next[idx] = next_cell;
            }
        }
        self.cells = next;
    }

    pub fn random(width: u32, height: u32, seed: u64) -> Self {
        // deterministic pseudo-rand to keep reproducible. xorshift64*
        let mut u = seed.wrapping_add(0x9E3779B97F4A7C15);
        let mut universe = Universe::new(width, height);
        for i in 0..(width*height) as usize {
            // xorshift64*
            u ^= u >> 12;
            u ^= u << 25;
            u ^= u >> 27;
            let r = u.wrapping_mul(2685821657736338717u64);
            universe.cells[i] = (r & 1) as u8;
        }
        universe
    }

    pub fn as_string(&self) -> String {
        // '.' dead, '█' alive
        let mut s = String::with_capacity((self.width * (self.height + 1)) as usize);
        for row in 0..self.height {
            for col in 0..self.width {
                let ch = if self.get(row, col) == 1 { '█' } else { '.' };
                s.push(ch);
            }
            s.push('\n');
        }
        s
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn blinker_oscillates() {
        // 5x5 with a horizontal blinker in the middle
        let mut u = Universe::new(5, 5);
        u.set(2, 1, true);
        u.set(2, 2, true);
        u.set(2, 3, true);

        let before = u.as_string();
        u.tick();
        let after = u.as_string();
        // after should be vertical blinker
        assert!(after.contains("..█.."));
        u.tick();
        assert_eq!(u.as_string(), before);
    }

    #[test]
    fn underpopulation_kills() {
        let mut u = Universe::new(3, 3);
        u.set(1, 1, true);
        u.tick();
        assert_eq!(u.get(1,1), 0);
    }

    #[test]
    fn wraparound_neighbors() {
        let mut u = Universe::new(3, 3);
        // put alive corner neighbors so center sees them via wrap
        u.set(0, 0, true);
        u.set(0, 2, true);
        u.set(2, 0, true);
        // center (1,1) should have 3 live neighbors
        assert_eq!(u.live_neighbor_count(1,1), 3);
    }
}
