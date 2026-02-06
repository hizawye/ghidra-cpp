# Project Status

## Current Progress
- Bootstrapped repo and docs per AGENTS instructions.
- Added C++20/CMake skeleton with core module stubs and app entrypoints.
- Cloned upstream Ghidra reference source to `ghidra-src/`.
- Implemented minimal ELF64 loader (headers + PT_LOAD regions).
- Added ELF section + symbol table parsing to populate Program symbols/types.
- Added memory image + ELF64 x86_64 relocation parsing and application.
- Added DWARF v4+ parsing for types, functions, and line info (basic forms).
- Added placeholder SLEIGH decoder and wired to headless CLI.

## Blockers/Bugs
- DWARF parser is minimal and does not handle all forms/tags.
- Decoder emits placeholder p-code only.
- No real decompiler logic yet.

## Next Immediate Starting Point
- Expand DWARF form/tag coverage and cross-reference types.
