# Project Status

## Current Progress
- Bootstrapped repo and docs per AGENTS instructions.
- Added C++20/CMake skeleton with core module stubs and app entrypoints.
- Cloned upstream Ghidra reference source to `ghidra-src/`.
- Implemented minimal ELF64 loader (headers + PT_LOAD regions).
- Added ELF section + symbol table parsing to populate Program symbols/types.
- Added memory image + ELF64 x86_64 relocation parsing and application.
- Added DWARF v4+ parsing for types, functions, and line info (basic forms).
- Expanded DWARF parsing with more tags/forms (ref, block, const/volatile/enum types).
- Added recursive DWARF type resolution and mapped debug types into Program type system.
- Added struct/union members to Program type system from DWARF.
- Added DWARF bitfield/alignment metadata for struct/union members.
- Added ELF section/segment tables to Program and loader.
- Implemented PE loader (sections, imports/exports, base relocs, PDB path).
- Added placeholder SLEIGH decoder and wired to headless CLI.

## Blockers/Bugs
- Mach-O loader still missing.
- DWARF parser is still partial and does not handle all alignment/bitfield edge cases.
- Decoder emits placeholder p-code only.
- No real decompiler logic yet.

## Next Immediate Starting Point
- Implement Mach-O loader (segments, symbols, relocations).
