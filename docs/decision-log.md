## 2026-02-06
- Initialized C++20 + CMake skeleton for Ghidra port.
- Selected pure C++ implementation with Dear ImGui (UI) and Lua (scripting) placeholders.
- Cloned upstream Ghidra as read-only reference in `ghidra-src/`.
## 2026-02-06
- Implemented minimal ELF64 loader (PT_LOAD regions only) and placeholder decoder wiring for headless CLI.
## 2026-02-06
- Added ELF section header and symbol table parsing to populate Program symbols and basic data types.
## 2026-02-06
- Added memory image and ELF64 x86_64 relocation parsing + application in loader.
## 2026-02-06
- Added DebugInfo structure and minimal DWARF v4+ parsing for types, functions, and line info.
## 2026-02-06
- Expanded DWARF v4+ parsing with additional tags/forms and basic type references.
## 2026-02-06
- Added basic DWARF type reference resolution and mapped debug types into Program type system.
## 2026-02-06
- Added recursive DWARF type resolution for pointers/arrays/typedef/const/volatile.
## 2026-02-06
- Added struct/union member support in Program type system and DWARF mapping.
## 2026-02-06
- Added DWARF bitfield/alignment metadata for struct/union members.
## 2026-02-06
- Added section/segment tables to Program and populated them in ELF loader.
