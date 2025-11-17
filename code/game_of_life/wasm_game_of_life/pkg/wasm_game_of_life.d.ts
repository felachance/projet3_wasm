/* tslint:disable */
/* eslint-disable */
export class Universe {
  free(): void;
  [Symbol.dispose](): void;
  toggle_cell(row: number, col: number): void;
  constructor(width: number, height: number);
  tick(): void;
  clear(): void;
  width(): number;
  height(): number;
  get_cells(): Uint8Array;
  randomize(): void;
}

export type InitInput = RequestInfo | URL | Response | BufferSource | WebAssembly.Module;

export interface InitOutput {
  readonly memory: WebAssembly.Memory;
  readonly __wbg_universe_free: (a: number, b: number) => void;
  readonly universe_clear: (a: number) => void;
  readonly universe_get_cells: (a: number) => [number, number];
  readonly universe_height: (a: number) => number;
  readonly universe_new: (a: number, b: number) => number;
  readonly universe_randomize: (a: number) => void;
  readonly universe_tick: (a: number) => void;
  readonly universe_toggle_cell: (a: number, b: number, c: number) => void;
  readonly universe_width: (a: number) => number;
  readonly __wbindgen_externrefs: WebAssembly.Table;
  readonly __wbindgen_free: (a: number, b: number, c: number) => void;
  readonly __wbindgen_start: () => void;
}

export type SyncInitInput = BufferSource | WebAssembly.Module;
/**
* Instantiates the given `module`, which can either be bytes or
* a precompiled `WebAssembly.Module`.
*
* @param {{ module: SyncInitInput }} module - Passing `SyncInitInput` directly is deprecated.
*
* @returns {InitOutput}
*/
export function initSync(module: { module: SyncInitInput } | SyncInitInput): InitOutput;

/**
* If `module_or_path` is {RequestInfo} or {URL}, makes a request and
* for everything else, calls `WebAssembly.instantiate` directly.
*
* @param {{ module_or_path: InitInput | Promise<InitInput> }} module_or_path - Passing `InitInput` directly is deprecated.
*
* @returns {Promise<InitOutput>}
*/
export default function __wbg_init (module_or_path?: { module_or_path: InitInput | Promise<InitInput> } | InitInput | Promise<InitInput>): Promise<InitOutput>;
