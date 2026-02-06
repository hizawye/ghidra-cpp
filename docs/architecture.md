# Architecture

## Modules
- libcore: program model, memory map, symbols, type system, memory image, relocations, debug info
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
- Loader parses section headers and symbol tables to populate Program symbols/types.
- Loader builds memory image from PT_LOAD segments and applies ELF64 x86_64 relocations.
- Loader parses DWARF v4+ for types, functions, and line info.
- Decoder emits placeholder p-code for a byte stream.
