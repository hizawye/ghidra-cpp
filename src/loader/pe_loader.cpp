#include "ghirda/loader/pe_loader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "ghirda/core/address_space.h"
#include "ghirda/core/memory_map.h"
#include "ghirda/core/relocation.h"

namespace ghirda::loader {
namespace {

#pragma pack(push, 1)
struct DosHeader {
  uint16_t e_magic;
  uint16_t e_cblp;
  uint16_t e_cp;
  uint16_t e_crlc;
  uint16_t e_cparhdr;
  uint16_t e_minalloc;
  uint16_t e_maxalloc;
  uint16_t e_ss;
  uint16_t e_sp;
  uint16_t e_csum;
  uint16_t e_ip;
  uint16_t e_cs;
  uint16_t e_lfarlc;
  uint16_t e_ovno;
  uint16_t e_res[4];
  uint16_t e_oemid;
  uint16_t e_oeminfo;
  uint16_t e_res2[10];
  uint32_t e_lfanew;
};

struct FileHeader {
  uint16_t machine;
  uint16_t number_of_sections;
  uint32_t time_date_stamp;
  uint32_t pointer_to_symbol_table;
  uint32_t number_of_symbols;
  uint16_t size_of_optional_header;
  uint16_t characteristics;
};

struct DataDirectory {
  uint32_t virtual_address;
  uint32_t size;
};

struct OptionalHeader32 {
  uint16_t magic;
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t size_of_code;
  uint32_t size_of_initialized_data;
  uint32_t size_of_uninitialized_data;
  uint32_t address_of_entry_point;
  uint32_t base_of_code;
  uint32_t base_of_data;
  uint32_t image_base;
  uint32_t section_alignment;
  uint32_t file_alignment;
  uint16_t major_os_version;
  uint16_t minor_os_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version;
  uint16_t minor_subsystem_version;
  uint32_t win32_version_value;
  uint32_t size_of_image;
  uint32_t size_of_headers;
  uint32_t checksum;
  uint16_t subsystem;
  uint16_t dll_characteristics;
  uint32_t size_of_stack_reserve;
  uint32_t size_of_stack_commit;
  uint32_t size_of_heap_reserve;
  uint32_t size_of_heap_commit;
  uint32_t loader_flags;
  uint32_t number_of_rva_and_sizes;
  DataDirectory data_directory[16];
};

struct OptionalHeader64 {
  uint16_t magic;
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t size_of_code;
  uint32_t size_of_initialized_data;
  uint32_t size_of_uninitialized_data;
  uint32_t address_of_entry_point;
  uint32_t base_of_code;
  uint64_t image_base;
  uint32_t section_alignment;
  uint32_t file_alignment;
  uint16_t major_os_version;
  uint16_t minor_os_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version;
  uint16_t minor_subsystem_version;
  uint32_t win32_version_value;
  uint32_t size_of_image;
  uint32_t size_of_headers;
  uint32_t checksum;
  uint16_t subsystem;
  uint16_t dll_characteristics;
  uint64_t size_of_stack_reserve;
  uint64_t size_of_stack_commit;
  uint64_t size_of_heap_reserve;
  uint64_t size_of_heap_commit;
  uint32_t loader_flags;
  uint32_t number_of_rva_and_sizes;
  DataDirectory data_directory[16];
};

struct SectionHeader {
  char name[8];
  uint32_t virtual_size;
  uint32_t virtual_address;
  uint32_t size_of_raw_data;
  uint32_t pointer_to_raw_data;
  uint32_t pointer_to_relocations;
  uint32_t pointer_to_linenumbers;
  uint16_t number_of_relocations;
  uint16_t number_of_linenumbers;
  uint32_t characteristics;
};

struct ImportDescriptor {
  uint32_t original_first_thunk;
  uint32_t time_date_stamp;
  uint32_t forwarder_chain;
  uint32_t name;
  uint32_t first_thunk;
};

struct ExportDirectory {
  uint32_t characteristics;
  uint32_t time_date_stamp;
  uint16_t major_version;
  uint16_t minor_version;
  uint32_t name;
  uint32_t base;
  uint32_t number_of_functions;
  uint32_t number_of_names;
  uint32_t address_of_functions;
  uint32_t address_of_names;
  uint32_t address_of_name_ordinals;
};

struct BaseRelocBlock {
  uint32_t page_rva;
  uint32_t block_size;
};

struct DebugDirectory {
  uint32_t characteristics;
  uint32_t time_date_stamp;
  uint16_t major_version;
  uint16_t minor_version;
  uint32_t type;
  uint32_t size_of_data;
  uint32_t address_of_raw_data;
  uint32_t pointer_to_raw_data;
};
#pragma pack(pop)

constexpr uint16_t kDosMagic = 0x5a4d;
constexpr uint32_t kNtSignature = 0x00004550; // PE\0\0
constexpr uint16_t kOptMagic32 = 0x10b;
constexpr uint16_t kOptMagic64 = 0x20b;

constexpr uint32_t kDirExport = 0;
constexpr uint32_t kDirImport = 1;
constexpr uint32_t kDirReloc = 5;
constexpr uint32_t kDirDebug = 6;

constexpr uint32_t kRelocHighLow = 3;
constexpr uint32_t kRelocDir64 = 10;

constexpr uint32_t kDebugTypeCodeView = 2;

bool read_exact(std::ifstream& in, void* data, size_t size) {
  in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
  return in.good();
}

bool read_blob(std::ifstream& in, uint32_t file_offset, uint32_t size, std::vector<uint8_t>* out) {
  out->assign(size, 0);
  in.seekg(static_cast<std::streamoff>(file_offset), std::ios::beg);
  if (!in) {
    return false;
  }
  in.read(reinterpret_cast<char*>(out->data()), static_cast<std::streamsize>(size));
  return in.good();
}

uint32_t rva_to_file_offset(uint32_t rva, uint32_t headers_size, const std::vector<SectionHeader>& sections) {
  if (rva < headers_size) {
    return rva;
  }
  for (const auto& sec : sections) {
    uint32_t start = sec.virtual_address;
    uint32_t end = sec.virtual_address + std::max(sec.virtual_size, sec.size_of_raw_data);
    if (rva >= start && rva < end) {
      return sec.pointer_to_raw_data + (rva - sec.virtual_address);
    }
  }
  return 0;
}

std::string read_string_at(std::ifstream& in, uint32_t file_offset) {
  in.seekg(static_cast<std::streamoff>(file_offset), std::ios::beg);
  if (!in) {
    return {};
  }
  std::string out;
  char ch = 0;
  while (in.get(ch)) {
    if (ch == '\0') {
      break;
    }
    out.push_back(ch);
  }
  return out;
}

} // namespace

bool PeLoader::load(const std::string& path, ghirda::core::Program* program, std::string* error) {
  if (!program) {
    if (error) {
      *error = "program output is null";
    }
    return false;
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) {
      *error = "failed to open file";
    }
    return false;
  }

  DosHeader dos{};
  if (!read_exact(in, &dos, sizeof(dos)) || dos.e_magic != kDosMagic) {
    if (error) {
      *error = "invalid DOS header";
    }
    return false;
  }

  in.seekg(static_cast<std::streamoff>(dos.e_lfanew), std::ios::beg);
  uint32_t signature = 0;
  if (!read_exact(in, &signature, sizeof(signature)) || signature != kNtSignature) {
    if (error) {
      *error = "invalid NT signature";
    }
    return false;
  }

  FileHeader file_header{};
  if (!read_exact(in, &file_header, sizeof(file_header))) {
    if (error) {
      *error = "failed to read file header";
    }
    return false;
  }

  std::vector<uint8_t> optional_raw(file_header.size_of_optional_header);
  if (!optional_raw.empty()) {
    in.read(reinterpret_cast<char*>(optional_raw.data()), file_header.size_of_optional_header);
    if (!in) {
      if (error) {
        *error = "failed to read optional header";
      }
      return false;
    }
  }

  bool is_pe32 = false;
  uint64_t image_base = 0;
  uint32_t entry_point = 0;
  uint32_t headers_size = 0;
  DataDirectory dirs[16] = {};
  if (optional_raw.size() >= sizeof(uint16_t)) {
    uint16_t magic = *reinterpret_cast<uint16_t*>(optional_raw.data());
    if (magic == kOptMagic32 && optional_raw.size() >= sizeof(OptionalHeader32)) {
      const auto* opt = reinterpret_cast<const OptionalHeader32*>(optional_raw.data());
      is_pe32 = true;
      image_base = opt->image_base;
      entry_point = opt->address_of_entry_point;
      headers_size = opt->size_of_headers;
      std::copy(std::begin(opt->data_directory), std::end(opt->data_directory), dirs);
    } else if (magic == kOptMagic64 && optional_raw.size() >= sizeof(OptionalHeader64)) {
      const auto* opt = reinterpret_cast<const OptionalHeader64*>(optional_raw.data());
      image_base = opt->image_base;
      entry_point = opt->address_of_entry_point;
      headers_size = opt->size_of_headers;
      std::copy(std::begin(opt->data_directory), std::end(opt->data_directory), dirs);
    } else {
      if (error) {
        *error = "unsupported optional header";
      }
      return false;
    }
  }

  std::vector<SectionHeader> sections(file_header.number_of_sections);
  for (uint16_t i = 0; i < file_header.number_of_sections; ++i) {
    if (!read_exact(in, &sections[i], sizeof(SectionHeader))) {
      if (error) {
        *error = "failed to read section headers";
      }
      return false;
    }
  }

  uint64_t min_vaddr = UINT64_MAX;
  uint64_t max_vaddr = 0;

  for (const auto& sec : sections) {
    ghirda::core::Program::Section s{};
    s.name = std::string(sec.name, sec.name + 8);
    s.name.erase(std::find(s.name.begin(), s.name.end(), '\0'), s.name.end());
    s.address = image_base + sec.virtual_address;
    s.size = sec.virtual_size;
    s.file_offset = sec.pointer_to_raw_data;
    s.flags = sec.characteristics;
    if (!s.name.empty()) {
      program->add_section(s);
    }

    ghirda::core::Program::Segment seg{};
    seg.vaddr = image_base + sec.virtual_address;
    seg.memsz = sec.virtual_size;
    seg.filesz = sec.size_of_raw_data;
    seg.flags = sec.characteristics;
    program->add_segment(seg);

    ghirda::core::MemoryRegion region{};
    region.start = seg.vaddr;
    region.size = seg.memsz;
    region.readable = (sec.characteristics & 0x40000000u) != 0;
    region.writable = (sec.characteristics & 0x80000000u) != 0;
    region.executable = (sec.characteristics & 0x20000000u) != 0;
    program->memory_map().add_region(region);

    min_vaddr = std::min(min_vaddr, seg.vaddr);
    max_vaddr = std::max(max_vaddr, seg.vaddr + seg.memsz);

    if (sec.size_of_raw_data != 0) {
      std::vector<uint8_t> bytes;
      if (!read_blob(in, sec.pointer_to_raw_data, sec.size_of_raw_data, &bytes)) {
        if (error) {
          *error = "failed to read section data";
        }
        return false;
      }
      program->memory_image().map_segment(seg.vaddr, bytes);
      if (sec.virtual_size > sec.size_of_raw_data) {
        program->memory_image().zero_fill(seg.vaddr + sec.size_of_raw_data,
                                          sec.virtual_size - sec.size_of_raw_data);
      }
    }
  }

  if (min_vaddr < max_vaddr) {
    program->add_address_space(ghirda::core::AddressSpace("image", min_vaddr, max_vaddr - min_vaddr));
  }
  program->set_load_bias(image_base);

  if (dirs[kDirExport].virtual_address != 0) {
    uint32_t export_offset = rva_to_file_offset(dirs[kDirExport].virtual_address, headers_size, sections);
    if (export_offset != 0) {
      ExportDirectory exp{};
      in.seekg(static_cast<std::streamoff>(export_offset), std::ios::beg);
      if (read_exact(in, &exp, sizeof(exp))) {
        uint32_t names_offset = rva_to_file_offset(exp.address_of_names, headers_size, sections);
        uint32_t ord_offset = rva_to_file_offset(exp.address_of_name_ordinals, headers_size, sections);
        uint32_t func_offset = rva_to_file_offset(exp.address_of_functions, headers_size, sections);
        if (names_offset && ord_offset && func_offset) {
          std::vector<uint32_t> name_rvas(exp.number_of_names);
          in.seekg(static_cast<std::streamoff>(names_offset), std::ios::beg);
          in.read(reinterpret_cast<char*>(name_rvas.data()), exp.number_of_names * sizeof(uint32_t));
          std::vector<uint16_t> ordinals(exp.number_of_names);
          in.seekg(static_cast<std::streamoff>(ord_offset), std::ios::beg);
          in.read(reinterpret_cast<char*>(ordinals.data()), exp.number_of_names * sizeof(uint16_t));
          std::vector<uint32_t> funcs(exp.number_of_functions);
          in.seekg(static_cast<std::streamoff>(func_offset), std::ios::beg);
          in.read(reinterpret_cast<char*>(funcs.data()), exp.number_of_functions * sizeof(uint32_t));

          for (size_t i = 0; i < name_rvas.size(); ++i) {
            uint32_t name_offset = rva_to_file_offset(name_rvas[i], headers_size, sections);
            std::string name = read_string_at(in, name_offset);
            if (name.empty()) {
              continue;
            }
            uint16_t ord = ordinals[i];
            if (ord >= funcs.size()) {
              continue;
            }
            ghirda::core::Symbol sym{};
            sym.name = name;
            sym.address = image_base + funcs[ord];
            sym.kind = ghirda::core::SymbolKind::Function;
            program->add_symbol(sym);
          }
        }
      }
    }
  }

  if (dirs[kDirImport].virtual_address != 0) {
    uint32_t imp_offset = rva_to_file_offset(dirs[kDirImport].virtual_address, headers_size, sections);
    if (imp_offset != 0) {
      in.seekg(static_cast<std::streamoff>(imp_offset), std::ios::beg);
      while (true) {
        ImportDescriptor desc{};
        if (!read_exact(in, &desc, sizeof(desc))) {
          break;
        }
        std::streamoff next_desc = in.tellg();
        if (desc.name == 0) {
          break;
        }
        std::string dll = read_string_at(in, rva_to_file_offset(desc.name, headers_size, sections));
        uint32_t thunk_rva = desc.original_first_thunk ? desc.original_first_thunk : desc.first_thunk;
        uint32_t thunk_offset = rva_to_file_offset(thunk_rva, headers_size, sections);
        if (thunk_offset == 0) {
          in.seekg(next_desc, std::ios::beg);
          continue;
        }
        in.seekg(static_cast<std::streamoff>(thunk_offset), std::ios::beg);
        while (true) {
          uint64_t thunk = 0;
          if (is_pe32) {
            uint32_t t32 = 0;
            if (!read_exact(in, &t32, sizeof(t32))) {
              break;
            }
            thunk = t32;
          } else {
            if (!read_exact(in, &thunk, sizeof(thunk))) {
              break;
            }
          }
          if (thunk == 0) {
            break;
          }
          if ((thunk & (is_pe32 ? 0x80000000u : 0x8000000000000000ull)) != 0) {
            continue;
          }
          uint32_t hint_name_rva = static_cast<uint32_t>(thunk);
          uint32_t hint_name_offset = rva_to_file_offset(hint_name_rva, headers_size, sections);
          if (hint_name_offset == 0) {
            continue;
          }
          in.seekg(static_cast<std::streamoff>(hint_name_offset + 2), std::ios::beg);
          std::string func = read_string_at(in, hint_name_offset + 2);
          if (!func.empty()) {
            ghirda::core::Symbol sym{};
            sym.name = dll + "!" + func;
            sym.address = image_base + thunk_rva;
            sym.kind = ghirda::core::SymbolKind::External;
            program->add_symbol(sym);
          }
        }
        in.seekg(next_desc, std::ios::beg);
      }
    }
  }

  if (dirs[kDirReloc].virtual_address != 0) {
    uint32_t reloc_offset = rva_to_file_offset(dirs[kDirReloc].virtual_address, headers_size, sections);
    uint32_t reloc_size = dirs[kDirReloc].size;
    if (reloc_offset != 0 && reloc_size != 0) {
      uint32_t cursor = reloc_offset;
      uint32_t end = reloc_offset + reloc_size;
      while (cursor + sizeof(BaseRelocBlock) <= end) {
        BaseRelocBlock block{};
        in.seekg(static_cast<std::streamoff>(cursor), std::ios::beg);
        if (!read_exact(in, &block, sizeof(block)) || block.block_size == 0) {
          break;
        }
        uint32_t entry_count = (block.block_size - sizeof(BaseRelocBlock)) / sizeof(uint16_t);
        std::vector<uint16_t> entries(entry_count);
        in.read(reinterpret_cast<char*>(entries.data()), entry_count * sizeof(uint16_t));
        for (uint16_t entry : entries) {
          uint16_t type = entry >> 12;
          uint16_t offset = entry & 0x0fff;
          uint64_t addr = image_base + block.page_rva + offset;
          ghirda::core::Relocation reloc{};
          reloc.address = addr;
          reloc.type = type;
          reloc.symbol = "";
          reloc.addend = 0;
          if (type == kRelocHighLow) {
            uint32_t value = 0;
            if (program->memory_image().read_u32(addr, &value)) {
              program->memory_image().write_u32(addr, value);
              reloc.applied = true;
            } else {
              reloc.note = "reloc read failed";
            }
          } else if (type == kRelocDir64) {
            uint64_t value = 0;
            if (program->memory_image().read_u64(addr, &value)) {
              program->memory_image().write_u64(addr, value);
              reloc.applied = true;
            } else {
              reloc.note = "reloc read failed";
            }
          } else {
            reloc.note = "unsupported reloc";
          }
          program->add_relocation(reloc);
        }
        cursor += block.block_size;
      }
    }
  }

  if (dirs[kDirDebug].virtual_address != 0) {
    uint32_t dbg_offset = rva_to_file_offset(dirs[kDirDebug].virtual_address, headers_size, sections);
    if (dbg_offset != 0) {
      size_t count = dirs[kDirDebug].size / sizeof(DebugDirectory);
      in.seekg(static_cast<std::streamoff>(dbg_offset), std::ios::beg);
      for (size_t i = 0; i < count; ++i) {
        DebugDirectory dbg{};
        if (!read_exact(in, &dbg, sizeof(dbg))) {
          break;
        }
        if (dbg.type == kDebugTypeCodeView && dbg.pointer_to_raw_data != 0) {
          std::vector<uint8_t> cv;
          if (read_blob(in, dbg.pointer_to_raw_data, dbg.size_of_data, &cv) && cv.size() > 24) {
            if (cv[0] == 'R' && cv[1] == 'S' && cv[2] == 'D' && cv[3] == 'S') {
              std::string path(reinterpret_cast<const char*>(cv.data() + 24));
              program->debug_info().pdb_path = path;
            }
          }
        }
      }
    }
  }

  (void)entry_point;
  return true;
}

} // namespace ghirda::loader
