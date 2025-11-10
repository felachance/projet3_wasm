use wasm_bindgen::prelude::*;
use js_sys::Math;

#[wasm_bindgen]
pub struct Universe {
    width: u32,
    height: u32,
    cells: Vec<u8>,
}

#[wasm_bindgen]
impl Universe {
    #[wasm_bindgen(constructor)]
    pub fn new(width: u32, height: u32) -> Universe {
        let mut cells = Vec::with_capacity((width * height) as usize);
        for _ in 0..width*height {
            cells.push(if Math::random() < 0.5 { 1 } else { 0 });
        }
        Universe { width, height, cells }
    }

    #[wasm_bindgen]
    pub fn tick(&mut self) {
        let mut next = self.cells.clone();
        for row in 0..self.height {
            for col in 0..self.width {
                let idx = (row * self.width + col) as usize;
                let live_neighbors = self.live_neighbors(row, col);
                next[idx] = match (self.cells[idx], live_neighbors) {
                    (1, 2) | (1, 3) => 1,
                    (1, _) => 0,
                    (0, 3) => 1,
                    (c, _) => c,
                };
            }
        }
        self.cells = next;
    }

    #[wasm_bindgen]
    pub fn randomize(&mut self) {
        for cell in self.cells.iter_mut() {
            *cell = if Math::random() < 0.5 { 1 } else { 0 };
        }
    }

    #[wasm_bindgen]
    pub fn clear(&mut self) {
        for cell in self.cells.iter_mut() {
            *cell = 0;
        }
    }

    fn live_neighbors(&self, row: u32, col: u32) -> u8 {
        let mut count = 0;
        for dr in [self.height-1,0,1] {
            for dc in [self.width-1,0,1] {
                if dr == 0 && dc == 0 { continue; }
                let r = (row+dr)%self.height;
                let c = (col+dc)%self.width;
                count += self.cells[(r*self.width + c) as usize];
            }
        }
        count
    }

    #[wasm_bindgen]
    pub fn get_cells(&self) -> Vec<u8> {
        self.cells.clone()
    }

    #[wasm_bindgen]
    pub fn toggle_cell(&mut self, row: u32, col: u32) {
        let idx = (row * self.width + col) as usize;
        if idx < self.cells.len() {
            self.cells[idx] ^= 1; // flip 0↔1
        }
    }

    #[wasm_bindgen]
    pub fn width(&self) -> u32 { self.width }

    #[wasm_bindgen]
    pub fn height(&self) -> u32 { self.height }
}
