# Architecture

## Modules
- libcore: program model, memory map, symbols, type system
- libsleigh: SLEIGH compiler, p-code IR, decoder
- libdecompiler: SSA, rule engine, decompiler pipeline
- libloader: ELF/PE/Mach-O loaders
- libui: GUI shell (Dear ImGui planned)
- libscript: Lua scripting runtime (planned)
- libplugin: plugin registry + ABI
- libserver: collaboration server (planned)

## Binaries
- ghidra_gui
- ghidra_headless
- ghidra_server
- sleighc
- ghidra_export

## Minimal ELF Flow
- ghidra_headless loads ELF64 (little endian) and populates memory map regions.
- Decoder emits placeholder p-code for a byte stream.
