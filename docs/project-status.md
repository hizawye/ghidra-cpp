# Project Status

## Current Progress
- Bootstrapped repo and docs per AGENTS instructions.
- Added C++20/CMake skeleton with core module stubs and app entrypoints.
- Cloned upstream Ghidra reference source to `ghidra-src/`.

## Blockers/Bugs
- All major subsystems are placeholders; no real loaders, decompiler, UI, or scripting runtime implemented.

## Next Immediate Starting Point
- Implement real ELF loader and minimal SLEIGH decode pipeline, then wire into headless CLI.
