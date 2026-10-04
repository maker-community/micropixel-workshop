// SPDX-License-Identifier: MIT
// Runs a WASI preview1 module, stubbing any host import the module declares
// but never needs (the SDK's device imports are linked with --allow-undefined).
import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const [file, ...args] = process.argv.slice(2);
const wasi = new WASI({ version: 'preview1', args: ['sim', ...args], env: {} });
const module = await WebAssembly.compile(await readFile(file));
const imports = { wasi_snapshot_preview1: wasi.wasiImport };
for (const entry of WebAssembly.Module.imports(module)) {
    if (entry.module === 'wasi_snapshot_preview1' || entry.kind !== 'function') continue;
    imports[entry.module] ??= {};
    imports[entry.module][entry.name] = () => {
        throw new Error(`unexpected host call ${entry.module}.${entry.name}`);
    };
}
const instance = await WebAssembly.instantiate(module, imports);
wasi.start(instance);
