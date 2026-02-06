# Project Status

## Current Progress
- Bootstrapped repo and docs per AGENTS instructions.
- Added C++20/CMake skeleton with core module stubs and app entrypoints.
- Cloned upstream Ghidra reference source to `ghidra-src/`.
- Implemented minimal ELF64 loader (headers + PT_LOAD regions).
- Added placeholder SLEIGH decoder and wired to headless CLI.

## Blockers/Bugs
- ELF loader does not parse sections, symbols, or relocations.
- Decoder emits placeholder p-code only.
- No real decompiler logic yet.

## Next Immediate Starting Point
- Implement ELF section/symbol parsing and plumb into Program symbols/types.
