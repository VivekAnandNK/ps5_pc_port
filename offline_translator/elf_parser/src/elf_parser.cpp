// =============================================================================
// elf_parser.cpp — PS5 ELF Parser implementation
// =============================================================================

#include "elf_parser.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ps5 {

// =============================================================================
// Public API
// =============================================================================

bool ElfParser::parse(const std::string& path) {
    // ── 1. Load file into memory ─────────────────────────────────────────────
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Cannot open file: " + path;
        return false;
    }

    auto file_size = static_cast<size_t>(file.tellg());
    if (file_size < sizeof(Elf64_Ehdr)) {
        error_ = "File too small to be a valid ELF64 (" +
                 std::to_string(file_size) + " bytes, need at least " +
                 std::to_string(sizeof(Elf64_Ehdr)) + ")";
        return false;
    }

    file.seekg(0);
    data_.resize(file_size);
    file.read(reinterpret_cast<char*>(data_.data()), static_cast<std::streamsize>(file_size));

    if (!file) {
        error_ = "Failed to read file contents";
        return false;
    }

    // ── 2. Parse ELF header ──────────────────────────────────────────────────
    if (!validate_header())       return false;

    // ── 3. Parse program headers (segments) ──────────────────────────────────
    if (!parse_program_headers()) return false;

    // ── 4. Parse dynamic sections (non-fatal — not all ELFs have them) ───────
    parse_dynamic_section();   // Standard PT_DYNAMIC
    parse_sce_dynlibdata();    // PS5-specific PT_SCE_DYNLIBDATA

    return true;
}

// =============================================================================
// Private helpers
// =============================================================================

template<typename T>
bool ElfParser::read_at(uint64_t offset, T& out) const {
    if (offset + sizeof(T) > data_.size()) return false;
    std::memcpy(&out, data_.data() + offset, sizeof(T));
    return true;
}

bool ElfParser::in_bounds(uint64_t offset, uint64_t size) const {
    return (offset + size) <= data_.size();
}

// =============================================================================
// Stage 1 — Validate ELF header
// =============================================================================

bool ElfParser::validate_header() {
    Elf64_Ehdr hdr;
    if (!read_at(0, hdr)) {
        error_ = "File too small for ELF header";
        return false;
    }

    // Magic bytes check
    if (hdr.e_ident[EI_MAG0] != ELFMAG0 ||
        hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        hdr.e_ident[EI_MAG2] != ELFMAG2 ||
        hdr.e_ident[EI_MAG3] != ELFMAG3) {
        error_ = "Not an ELF file (magic bytes mismatch: got "
                 + std::to_string(hdr.e_ident[0]) + " "
                 + std::to_string(hdr.e_ident[1]) + " "
                 + std::to_string(hdr.e_ident[2]) + " "
                 + std::to_string(hdr.e_ident[3]) + ")";
        return false;
    }

    // Must be 64-bit (PS5 is always 64-bit)
    if (hdr.e_ident[EI_CLASS] != ELFCLASS64) {
        error_ = "Only 64-bit ELF is supported "
                 "(PS5 uses 64-bit; got class " +
                 std::to_string((int)hdr.e_ident[EI_CLASS]) + ")";
        return false;
    }

    // Must be little-endian (PS5 and x86-64 are both little-endian)
    if (hdr.e_ident[EI_DATA] != ELFDATA2LSB) {
        error_ = "Only little-endian ELF is supported";
        return false;
    }

    // Populate ElfInfo
    info_.is_64bit         = true;
    info_.is_little_endian = true;
    info_.is_ps5           = (hdr.e_ident[EI_OSABI] == ELFOSABI_SCE);
    info_.os_abi           = hdr.e_ident[EI_OSABI];
    info_.type             = hdr.e_type;
    info_.machine          = hdr.e_machine;
    info_.entry_point      = hdr.e_entry;
    info_.raw_header       = hdr;

    return true;
}

// =============================================================================
// Stage 2 — Parse program headers (segment table)
// =============================================================================

bool ElfParser::parse_program_headers() {
    const auto& hdr = info_.raw_header;

    if (hdr.e_phoff == 0 || hdr.e_phnum == 0) {
        // No program headers — valid for some object files, unusual for executables
        return true;
    }

    if (hdr.e_phentsize != sizeof(Elf64_Phdr)) {
        error_ = "Unexpected program header entry size: got " +
                 std::to_string(hdr.e_phentsize) +
                 ", expected " + std::to_string(sizeof(Elf64_Phdr));
        return false;
    }

    if (!in_bounds(hdr.e_phoff, (uint64_t)hdr.e_phnum * sizeof(Elf64_Phdr))) {
        error_ = "Program header table is out of file bounds";
        return false;
    }

    for (uint16_t i = 0; i < hdr.e_phnum; ++i) {
        uint64_t offset = hdr.e_phoff + (uint64_t)i * sizeof(Elf64_Phdr);

        Elf64_Phdr phdr;
        if (!read_at(offset, phdr)) {
            error_ = "Failed to read program header " + std::to_string(i);
            return false;
        }

        Segment seg;
        seg.type      = phdr.p_type;
        seg.flags     = phdr.p_flags;
        seg.offset    = phdr.p_offset;
        seg.vaddr     = phdr.p_vaddr;
        seg.file_size = phdr.p_filesz;
        seg.mem_size  = phdr.p_memsz;
        seg.align     = phdr.p_align;

        info_.segments.push_back(seg);
        info_.raw_phdrs.push_back(phdr);
    }

    return true;
}

// =============================================================================
// Stage 3a — Parse standard PT_DYNAMIC section
//
// The standard ELF dynamic section contains DT_NEEDED entries that list
// required shared libraries. These reference a string table (DT_STRTAB)
// whose address is a virtual address within a PT_LOAD segment.
// =============================================================================

bool ElfParser::parse_dynamic_section() {
    // Find PT_DYNAMIC segment
    const Elf64_Phdr* dyn_phdr = nullptr;
    for (const auto& phdr : info_.raw_phdrs) {
        if (phdr.p_type == PT_DYNAMIC) { dyn_phdr = &phdr; break; }
    }
    if (!dyn_phdr) return false;
    if (!in_bounds(dyn_phdr->p_offset, dyn_phdr->p_filesz)) return false;

    const uint8_t* dyn_data = data_.data() + dyn_phdr->p_offset;
    size_t         num_dyn  = dyn_phdr->p_filesz / sizeof(Elf64_Dyn);

    // ── Pass 1: find DT_STRTAB (virtual addr) and DT_STRSZ ─────────────────
    uint64_t strtab_vaddr = 0;
    uint64_t strtab_size  = 0;

    for (size_t i = 0; i < num_dyn; ++i) {
        Elf64_Dyn dyn;
        std::memcpy(&dyn, dyn_data + i * sizeof(Elf64_Dyn), sizeof(Elf64_Dyn));
        if (dyn.d_tag == DT_NULL)   break;
        if (dyn.d_tag == DT_STRTAB) strtab_vaddr = dyn.d_val;
        if (dyn.d_tag == DT_STRSZ)  strtab_size  = dyn.d_val;
    }

    if (strtab_vaddr == 0 || strtab_size == 0) return false;

    // ── Translate virtual address → file offset ──────────────────────────────
    // Find the PT_LOAD segment that contains strtab_vaddr
    uint64_t strtab_file_off = 0;
    for (const auto& ph : info_.raw_phdrs) {
        if (ph.p_type != PT_LOAD) continue;
        if (strtab_vaddr >= ph.p_vaddr &&
            strtab_vaddr <  ph.p_vaddr + ph.p_filesz) {
            strtab_file_off = ph.p_offset + (strtab_vaddr - ph.p_vaddr);
            break;
        }
    }
    if (strtab_file_off == 0 || !in_bounds(strtab_file_off, strtab_size)) return false;

    const char* strtab = reinterpret_cast<const char*>(data_.data() + strtab_file_off);

    // ── Pass 2: extract DT_NEEDED entries ────────────────────────────────────
    for (size_t i = 0; i < num_dyn; ++i) {
        Elf64_Dyn dyn;
        std::memcpy(&dyn, dyn_data + i * sizeof(Elf64_Dyn), sizeof(Elf64_Dyn));
        if (dyn.d_tag == DT_NULL) break;

        if (dyn.d_tag == DT_NEEDED && dyn.d_val < strtab_size) {
            std::string libname(strtab + dyn.d_val);
            if (!libname.empty()) {
                // Deduplicate
                bool already_added = false;
                for (const auto& lib : info_.imports)
                    if (lib.name == libname) { already_added = true; break; }
                if (!already_added)
                    info_.imports.push_back({libname});
            }
        }
    }

    return true;
}

// =============================================================================
// Stage 3b — Parse PS5-specific PT_SCE_DYNLIBDATA segment
//
// PS5 stores its dynamic library info in a custom segment instead of relying
// on the standard ELF PT_DYNAMIC mechanism.
//
// The segment starts with an array of Elf64_Dyn entries (using DT_SCE_* tags).
// IMPORTANT: DT_SCE_STRTAB / DT_SCE_SYMTAB values are BYTE OFFSETS relative
// to the START of this segment — NOT virtual addresses (unlike standard ELF).
// =============================================================================

bool ElfParser::parse_sce_dynlibdata() {
    // Find PT_SCE_DYNLIBDATA segment
    const Elf64_Phdr* sce_phdr = nullptr;
    for (const auto& phdr : info_.raw_phdrs) {
        if (phdr.p_type == PT_SCE_DYNLIBDATA) { sce_phdr = &phdr; break; }
    }
    if (!sce_phdr) return false;
    if (!in_bounds(sce_phdr->p_offset, sce_phdr->p_filesz)) return false;

    const uint8_t* seg_base = data_.data() + sce_phdr->p_offset;
    size_t         num_dyn  = sce_phdr->p_filesz / sizeof(Elf64_Dyn);

    // ── Pass 1: scan for string/symbol table locations ────────────────────────
    uint64_t strtab_off  = 0;   // relative to seg_base
    uint64_t strtab_sz   = 0;
    uint64_t symtab_off  = 0;   // relative to seg_base
    uint64_t syment_sz   = sizeof(Elf64_Sym);
    uint64_t symtab_sz   = 0;

    for (size_t i = 0; i < num_dyn; ++i) {
        Elf64_Dyn dyn;
        std::memcpy(&dyn, seg_base + i * sizeof(Elf64_Dyn), sizeof(Elf64_Dyn));
        if (dyn.d_tag == DT_NULL) break;

        switch (dyn.d_tag) {
            case DT_SCE_STRTAB:  strtab_off = dyn.d_val; break;
            case DT_SCE_STRSZ:   strtab_sz  = dyn.d_val; break;
            case DT_SCE_SYMTAB:  symtab_off = dyn.d_val; break;
            case DT_SCE_SYMENT:  syment_sz  = dyn.d_val; break;
            case DT_SCE_SYMTABSZ: symtab_sz = dyn.d_val; break;
            default: break;
        }
    }

    // ── Validate and get pointers to string/symbol tables ────────────────────
    if (strtab_off == 0 || strtab_sz == 0) return false;
    if (!in_bounds(sce_phdr->p_offset + strtab_off, strtab_sz)) return false;

    const char* strtab = reinterpret_cast<const char*>(seg_base + strtab_off);

    // ── Pass 2: extract DT_NEEDED (library imports) ──────────────────────────
    for (size_t i = 0; i < num_dyn; ++i) {
        Elf64_Dyn dyn;
        std::memcpy(&dyn, seg_base + i * sizeof(Elf64_Dyn), sizeof(Elf64_Dyn));
        if (dyn.d_tag == DT_NULL) break;

        if (dyn.d_tag == DT_NEEDED && dyn.d_val < strtab_sz) {
            std::string libname(strtab + dyn.d_val);
            if (!libname.empty()) {
                bool already_added = false;
                for (const auto& lib : info_.imports)
                    if (lib.name == libname) { already_added = true; break; }
                if (!already_added)
                    info_.imports.push_back({libname});
            }
        }
    }

    // ── Parse symbol table ────────────────────────────────────────────────────
    if (symtab_off > 0 && syment_sz == sizeof(Elf64_Sym) && symtab_sz > 0 &&
        in_bounds(sce_phdr->p_offset + symtab_off, symtab_sz)) {

        const uint8_t* symtab_ptr = seg_base + symtab_off;
        size_t         num_syms   = symtab_sz / syment_sz;

        for (size_t i = 0; i < num_syms; ++i) {
            Elf64_Sym sym;
            std::memcpy(&sym, symtab_ptr + i * syment_sz, sizeof(Elf64_Sym));

            Symbol s;
            s.value         = sym.st_value;
            s.size          = sym.st_size;
            s.type          = ELF64_ST_TYPE(sym.st_info);
            s.binding       = ELF64_ST_BIND(sym.st_info);
            s.section_index = sym.st_shndx;

            // Resolve name from string table
            if (sym.st_name < strtab_sz) {
                s.name = std::string(strtab + sym.st_name);
            }

            info_.symbols.push_back(s);
        }
    }

    return true;
}

// =============================================================================
// String helpers
// =============================================================================

std::string ElfParser::flags_to_string(uint32_t flags) const {
    std::string s = "---";
    if (flags & PF_R) s[0] = 'R';
    if (flags & PF_W) s[1] = 'W';
    if (flags & PF_X) s[2] = 'X';
    return s;
}

std::string ElfParser::seg_type_to_string(uint32_t type) const {
    switch (type) {
        case PT_NULL:            return "PT_NULL";
        case PT_LOAD:            return "PT_LOAD";
        case PT_DYNAMIC:         return "PT_DYNAMIC";
        case PT_INTERP:          return "PT_INTERP";
        case PT_NOTE:            return "PT_NOTE";
        case PT_PHDR:            return "PT_PHDR";
        case PT_TLS:             return "PT_TLS";
        case PT_GNU_EH_FRAME:    return "PT_GNU_EH_FRAME";
        case PT_GNU_STACK:       return "PT_GNU_STACK";
        case PT_GNU_RELRO:       return "PT_GNU_RELRO";
        case PT_SCE_DYNLIBDATA:  return "PT_SCE_DYNLIBDATA";
        case PT_SCE_PROCPARAM:   return "PT_SCE_PROCPARAM";
        case PT_SCE_MODULEPARAM: return "PT_SCE_MODULEPARAM";
        case PT_SCE_RELRO:       return "PT_SCE_RELRO";
        case PT_SCE_COMMENT:     return "PT_SCE_COMMENT";
        case PT_SCE_LIBVERSION:  return "PT_SCE_LIBVERSION";
        default: {
            std::ostringstream ss;
            ss << "0x" << std::hex << std::uppercase << type;
            return ss.str();
        }
    }
}

std::string ElfParser::elf_type_to_string(uint16_t type) const {
    switch (type) {
        case ET_NONE:        return "ET_NONE";
        case ET_REL:         return "ET_REL (Relocatable)";
        case ET_EXEC:        return "ET_EXEC (Standard Executable)";
        case ET_DYN:         return "ET_DYN (Shared Library)";
        case ET_SCE_EXEC:    return "ET_SCE_EXEC (PS5 Executable)";
        case ET_SCE_RELEXEC: return "ET_SCE_RELEXEC (PS5 Relocatable Exec)";
        case ET_SCE_STUBLIB: return "ET_SCE_STUBLIB (PS5 Stub Library)";
        case ET_SCE_DYNEXEC: return "ET_SCE_DYNEXEC (PS5 PIE Executable)";
        case ET_SCE_DYNAMIC: return "ET_SCE_DYNAMIC (PS5 Dynamic Library .sprx)";
        default: {
            std::ostringstream ss;
            ss << "0x" << std::hex << std::uppercase << type << " (Unknown)";
            return ss.str();
        }
    }
}

std::string ElfParser::sym_type_to_string(uint8_t type) const {
    switch (type) {
        case STT_NOTYPE:  return "NOTYPE ";
        case STT_OBJECT:  return "OBJECT ";
        case STT_FUNC:    return "FUNC   ";
        case STT_SECTION: return "SECTION";
        case STT_FILE:    return "FILE   ";
        default:          return "OTHER  ";
    }
}

std::string ElfParser::sym_bind_to_string(uint8_t bind) const {
    switch (bind) {
        case STB_LOCAL:  return "LOCAL ";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK:   return "WEAK  ";
        default:         return "OTHER ";
    }
}

// =============================================================================
// dump() — Human-readable output
// =============================================================================

void ElfParser::dump() const {
    const std::string SEP(62, '-');

    std::cout << "\n";
    std::cout << "=============================================================\n";
    std::cout << "   PS5 ELF Parser v0.1  --  Phase 1 Output\n";
    std::cout << "=============================================================\n\n";

    std::cout << "  File size: " << data_.size() << " bytes"
              << " (" << (data_.size() / 1024) << " KB)\n\n";

    // ── ELF Header ───────────────────────────────────────────────────────────
    std::cout << SEP << "\n  ELF HEADER\n" << SEP << "\n";

    const auto& h = info_.raw_header;

    // Magic
    std::cout << "  Magic:        ";
    for (int i = 0; i < 4; ++i)
        std::cout << std::hex << std::uppercase
                  << std::setw(2) << std::setfill('0') << (int)h.e_ident[i] << " ";
    bool magic_ok = (h.e_ident[0]==ELFMAG0 && h.e_ident[1]==ELFMAG1 &&
                     h.e_ident[2]==ELFMAG2 && h.e_ident[3]==ELFMAG3);
    std::cout << std::dec << (magic_ok ? " [OK]" : " [INVALID]") << "\n";

    std::cout << "  Class:        " << (info_.is_64bit ? "64-bit" : "32-bit") << "\n";
    std::cout << "  Endianness:   " << (info_.is_little_endian ? "Little Endian" : "Big Endian") << "\n";

    std::cout << "  OS/ABI:       0x"
              << std::hex << std::setw(2) << std::setfill('0') << (int)info_.os_abi
              << std::dec;
    if (info_.is_ps5) std::cout << "  (SCE / PS5) [PS5 CONFIRMED]";
    else              std::cout << "  (Standard ELF)";
    std::cout << "\n";

    std::cout << "  Type:         0x"
              << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << info_.type
              << std::dec << "  " << elf_type_to_string(info_.type) << "\n";

    std::cout << "  Machine:      ";
    if (info_.machine == EM_X86_64) std::cout << "x86-64 [OK — matches PS5]";
    else std::cout << "0x" << std::hex << info_.machine << std::dec << " (unexpected)";
    std::cout << "\n";

    std::cout << "  Entry Point:  0x"
              << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
              << info_.entry_point << std::dec << "\n";

    std::cout << "  PH Offset:    0x"
              << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
              << h.e_phoff << std::dec << "\n";

    std::cout << "  PH Count:     " << h.e_phnum << "\n";
    std::cout << "  PH Ent Size:  " << h.e_phentsize << " bytes\n";

    // ── Program Headers ───────────────────────────────────────────────────────
    std::cout << "\n" << SEP << "\n"
              << "  PROGRAM HEADERS (" << info_.segments.size() << " segments)\n"
              << SEP << "\n";

    // Column header
    std::cout << "  "
              << std::left << std::setfill(' ') << std::setw(3)  << "#"
              << std::left << std::setw(22) << "Type"
              << std::left << std::setw(14) << "Offset"
              << std::left << std::setw(20) << "VAddr"
              << std::left << std::setw(14) << "FileSz"
              << std::left << std::setw(14) << "MemSz"
              << "Flags\n";
    std::cout << "  " << std::string(85, '-') << "\n";

    for (size_t i = 0; i < info_.segments.size(); ++i) {
        const auto& seg = info_.segments[i];

        std::cout << "  "
                  << std::left  << std::dec << std::setfill(' ') << std::setw(3) << i
                  << std::left  << std::setw(22) << seg_type_to_string(seg.type);

        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0')
                  << seg.offset << "    ";
        std::cout << "0x" << std::hex << std::right << std::setw(16) << std::setfill('0')
                  << seg.vaddr << "  ";
        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0')
                  << seg.file_size << "    ";
        std::cout << "0x" << std::hex << std::right << std::setw(8) << std::setfill('0')
                  << seg.mem_size << "  ";
        std::cout << std::left << std::setfill(' ') << flags_to_string(seg.flags);
        std::cout << std::dec << "\n";
    }

    // ── Imported Libraries ────────────────────────────────────────────────────
    std::cout << "\n" << SEP << "\n"
              << "  IMPORTED LIBRARIES (" << info_.imports.size() << ")\n"
              << SEP << "\n";

    if (info_.imports.empty()) {
        std::cout << "  (none found)\n";
    } else {
        for (const auto& lib : info_.imports)
            std::cout << "  --> " << lib.name << "\n";
    }

    // ── Symbols ───────────────────────────────────────────────────────────────
    if (!info_.symbols.empty()) {
        std::cout << "\n" << SEP << "\n"
                  << "  SYMBOLS (" << info_.symbols.size() << " entries, showing first 25)\n"
                  << SEP << "\n";

        std::cout << "  "
                  << std::left << std::setw(5)  << "Idx"
                  << std::left << std::setw(8)  << "Type"
                  << std::left << std::setw(8)  << "Bind"
                  << std::left << std::setw(20) << "Value"
                  << "Name\n";
        std::cout << "  " << std::string(70, '-') << "\n";

        size_t show = std::min(info_.symbols.size(), size_t(25));
        for (size_t i = 0; i < show; ++i) {
            const auto& sym = info_.symbols[i];
            std::cout << "  "
                      << std::left  << std::dec << std::setfill(' ') << std::setw(5) << i
                      << std::left  << std::setw(8) << sym_type_to_string(sym.type)
                      << std::left  << std::setw(8) << sym_bind_to_string(sym.binding)
                      << "0x" << std::hex << std::right << std::setw(16) << std::setfill('0')
                      << sym.value << std::dec << "  "
                      << (sym.name.empty() ? "(null)" : sym.name)
                      << "\n";
        }
        if (info_.symbols.size() > 25)
            std::cout << "  ... and " << (info_.symbols.size() - 25) << " more\n";
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    std::cout << "\n" << SEP << "\n  SUMMARY\n" << SEP << "\n";

    int  load_segs      = 0;
    bool has_sce_dynlib = false;
    bool has_dynamic    = false;

    for (const auto& seg : info_.segments) {
        if (seg.type == PT_LOAD)           load_segs++;
        if (seg.type == PT_SCE_DYNLIBDATA) has_sce_dynlib = true;
        if (seg.type == PT_DYNAMIC)        has_dynamic    = true;
    }

    std::cout << "  PS5 ELF (OS/ABI=SCE):   " << (info_.is_ps5 ? "YES" : "NO (standard ELF)") << "\n";
    std::cout << "  x86-64 Architecture:    " << (info_.machine == EM_X86_64 ? "YES" : "NO") << "\n";
    std::cout << "  Entry Point:            0x"
              << std::hex << std::uppercase << info_.entry_point << std::dec << "\n";
    std::cout << "  PT_LOAD segments:       " << load_segs << "\n";
    std::cout << "  PT_DYNAMIC present:     " << (has_dynamic    ? "YES" : "NO") << "\n";
    std::cout << "  PT_SCE_DYNLIBDATA:      " << (has_sce_dynlib ? "YES (PS5 specific)" : "NO") << "\n";
    std::cout << "  Library imports:        " << info_.imports.size() << "\n";
    std::cout << "  Symbols parsed:         " << info_.symbols.size() << "\n";
    std::cout << "\n  Result: PARSE SUCCESS\n\n";
}

} // namespace ps5

