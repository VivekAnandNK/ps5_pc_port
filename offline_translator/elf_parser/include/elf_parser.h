#pragma once
// =============================================================================
// elf_parser.h — PS5 ELF Parser class interface
//
// Usage:
//   ps5::ElfParser parser;
//   if (!parser.parse("game.elf")) {
//       std::cerr << parser.error() << "\n";
//       return 1;
//   }
//   parser.dump();
//   auto& info = parser.info();
//   // use info.entry_point, info.segments, info.imports, info.symbols ...
// =============================================================================

#include "ps5_elf_types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ps5 {

// -----------------------------------------------------------------------------
// Data structures returned by the parser
// -----------------------------------------------------------------------------

/// One program segment (PT_LOAD, PT_DYNAMIC, PT_SCE_DYNLIBDATA, etc.)
struct Segment {
    uint32_t type;       ///< PT_* constant
    uint32_t flags;      ///< Bitmask: PF_R | PF_W | PF_X
    uint64_t offset;     ///< Byte offset of segment data in the file
    uint64_t vaddr;      ///< Target virtual address when loaded into memory
    uint64_t file_size;  ///< Bytes occupied in the file (may be < mem_size)
    uint64_t mem_size;   ///< Bytes needed in memory (extra zeroed = BSS)
    uint64_t align;      ///< Alignment requirement (power of 2)
};

/// A shared library this binary imports from (e.g. "libSceKernel.sprx")
struct ImportedLibrary {
    std::string name;
};

/// One entry from the ELF symbol table
struct Symbol {
    std::string name;          ///< Symbol name (empty for null entry)
    uint64_t    value;         ///< Address / offset
    uint64_t    size;          ///< Size in bytes (0 if unknown)
    uint8_t     type;          ///< STT_NOTYPE, STT_FUNC, STT_OBJECT, ...
    uint8_t     binding;       ///< STB_LOCAL, STB_GLOBAL, STB_WEAK
    uint16_t    section_index; ///< SHN_UNDEF = imported; SHN_ABS = absolute
};

/// Full result of a successful parse
struct ElfInfo {
    // ── Basic header info ───────────────────────────────────────────────────
    bool     is_64bit;          ///< true (we reject 32-bit)
    bool     is_little_endian;  ///< true (PS5 and x86-64 are both LE)
    bool     is_ps5;            ///< true if OS/ABI == ELFOSABI_SCE (0x09)
    uint8_t  os_abi;            ///< Raw OSABI byte
    uint16_t type;              ///< ET_SCE_EXEC, ET_SCE_DYNAMIC, etc.
    uint16_t machine;           ///< Should be EM_X86_64 (62)
    uint64_t entry_point;       ///< Virtual address of program entry

    // ── Parsed data ─────────────────────────────────────────────────────────
    std::vector<Segment>         segments; ///< All program headers
    std::vector<ImportedLibrary> imports;  ///< Libraries this binary needs
    std::vector<Symbol>          symbols;  ///< Symbol table entries

    // ── Raw structures (for downstream modules: loader, disassembler, etc.) ─
    Elf64_Ehdr              raw_header;
    std::vector<Elf64_Phdr> raw_phdrs;
};

// -----------------------------------------------------------------------------
// ElfParser — main parser class
// -----------------------------------------------------------------------------
class ElfParser {
public:
    /// Load and parse a PS5 (or standard) ELF64 binary from disk.
    /// @return true on success; false on any error (call error() for reason).
    bool parse(const std::string& path);

    /// Parsed information — only valid after parse() returns true.
    const ElfInfo& info() const { return info_; }

    /// Human-readable error description when parse() returns false.
    const std::string& error() const { return error_; }

    /// Print a formatted, human-readable dump of all parsed data to stdout.
    void dump() const;

private:
    ElfInfo              info_;
    std::string          error_;
    std::vector<uint8_t> data_;  ///< Raw file bytes kept in memory

    // ── Parsing stages ──────────────────────────────────────────────────────
    bool validate_header();
    bool parse_program_headers();
    bool parse_dynamic_section();    ///< Standard PT_DYNAMIC
    bool parse_sce_dynlibdata();     ///< PS5-specific PT_SCE_DYNLIBDATA

    // ── Helpers ─────────────────────────────────────────────────────────────

    /// Read sizeof(T) bytes from offset into out. Returns false if out of bounds.
    template<typename T>
    bool read_at(uint64_t offset, T& out) const;

    /// True if [offset, offset+size) is fully within the file.
    bool in_bounds(uint64_t offset, uint64_t size) const;

    /// Convert p_flags bitmask to "RWX" / "R-X" / "R--" string.
    std::string flags_to_string(uint32_t flags) const;

    /// Convert p_type to a readable name ("PT_LOAD", "PT_SCE_DYNLIBDATA", etc.)
    std::string seg_type_to_string(uint32_t type) const;

    /// Convert e_type to a readable name ("ET_SCE_EXEC (PS5 Executable)", etc.)
    std::string elf_type_to_string(uint16_t type) const;

    /// Convert symbol type to a readable name ("FUNC", "OBJECT", etc.)
    std::string sym_type_to_string(uint8_t type) const;

    /// Convert symbol binding to a readable name ("GLOBAL", "LOCAL", etc.)
    std::string sym_bind_to_string(uint8_t bind) const;
};

} // namespace ps5

