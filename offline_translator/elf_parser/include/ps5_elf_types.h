#pragma once
// =============================================================================
// ps5_elf_types.h — PS5 ELF type definitions
//
// The PS5 uses standard ELF64 format with Sony-specific extensions:
//   - Custom e_type values  (ET_SCE_*)
//   - Custom OS/ABI value   (ELFOSABI_SCE = 0x09)
//   - Custom segment types  (PT_SCE_*)
//   - Custom dynamic tags   (DT_SCE_*) in PT_SCE_DYNLIBDATA
//
// All standard ELF64 structures are binary-compatible — the CPU is x86-64
// (little-endian), same as a PC, so no byte-swapping is needed.
// =============================================================================

#include <cstdint>

// -----------------------------------------------------------------------------
// ELF64 scalar type aliases (matches the ELF spec exactly)
// -----------------------------------------------------------------------------
using Elf64_Addr   = uint64_t;  ///< Virtual address
using Elf64_Off    = uint64_t;  ///< File offset
using Elf64_Half   = uint16_t;  ///< 16-bit unsigned
using Elf64_Word   = uint32_t;  ///< 32-bit unsigned
using Elf64_Sword  = int32_t;   ///< 32-bit signed
using Elf64_Xword  = uint64_t;  ///< 64-bit unsigned
using Elf64_Sxword = int64_t;   ///< 64-bit signed

// -----------------------------------------------------------------------------
// ELF Identification (e_ident indices)
// -----------------------------------------------------------------------------
constexpr int EI_MAG0    = 0;   ///< Magic byte 0 (0x7F)
constexpr int EI_MAG1    = 1;   ///< Magic byte 1 ('E')
constexpr int EI_MAG2    = 2;   ///< Magic byte 2 ('L')
constexpr int EI_MAG3    = 3;   ///< Magic byte 3 ('F')
constexpr int EI_CLASS   = 4;   ///< File class (32 or 64 bit)
constexpr int EI_DATA    = 5;   ///< Data encoding (endianness)
constexpr int EI_VERSION = 6;   ///< ELF version
constexpr int EI_OSABI   = 7;   ///< OS/ABI identifier
constexpr int EI_NIDENT  = 16;  ///< Size of e_ident

// Magic bytes
constexpr uint8_t ELFMAG0 = 0x7F;
constexpr uint8_t ELFMAG1 = 'E';
constexpr uint8_t ELFMAG2 = 'L';
constexpr uint8_t ELFMAG3 = 'F';

// EI_CLASS values
constexpr uint8_t ELFCLASS32 = 1;  ///< 32-bit ELF
constexpr uint8_t ELFCLASS64 = 2;  ///< 64-bit ELF (PS5 uses this)

// EI_DATA values
constexpr uint8_t ELFDATA2LSB = 1;  ///< Little-endian (PS5 and x86-64)
constexpr uint8_t ELFDATA2MSB = 2;  ///< Big-endian

// EI_OSABI values
constexpr uint8_t ELFOSABI_NONE = 0x00;  ///< Standard ELF
constexpr uint8_t ELFOSABI_SCE  = 0x09;  ///< Sony PlayStation (PS4/PS5)

// -----------------------------------------------------------------------------
// ELF Object File Types (e_type)
// -----------------------------------------------------------------------------
constexpr Elf64_Half ET_NONE        = 0x0000;  ///< No type
constexpr Elf64_Half ET_REL         = 0x0001;  ///< Relocatable
constexpr Elf64_Half ET_EXEC        = 0x0002;  ///< Standard executable
constexpr Elf64_Half ET_DYN         = 0x0003;  ///< Shared library
// PS5-specific types (Sony extensions in the OS-specific range 0xFE00–0xFEFF)
constexpr Elf64_Half ET_SCE_EXEC    = 0xFE00;  ///< PS5 executable
constexpr Elf64_Half ET_SCE_RELEXEC = 0xFE04;  ///< PS5 relocatable exec (PRX)
constexpr Elf64_Half ET_SCE_STUBLIB = 0xFE0C;  ///< PS5 stub library (.sprx stub)
constexpr Elf64_Half ET_SCE_DYNEXEC = 0xFE10;  ///< PS5 position-independent exec (PIE)
constexpr Elf64_Half ET_SCE_DYNAMIC = 0xFE18;  ///< PS5 dynamic library (.sprx)

// Machine type
constexpr Elf64_Half EM_X86_64 = 62;  ///< AMD/Intel x86-64

// -----------------------------------------------------------------------------
// ELF64 Header — 64 bytes total
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct Elf64_Ehdr {
    uint8_t    e_ident[EI_NIDENT]; ///< Magic, class, endian, version, OS/ABI
    Elf64_Half e_type;             ///< Object file type (ET_SCE_EXEC, etc.)
    Elf64_Half e_machine;          ///< Target architecture (EM_X86_64)
    Elf64_Word e_version;          ///< ELF version (always 1)
    Elf64_Addr e_entry;            ///< Entry point virtual address
    Elf64_Off  e_phoff;            ///< Program header table offset in file
    Elf64_Off  e_shoff;            ///< Section header table offset in file
    Elf64_Word e_flags;            ///< Processor-specific flags
    Elf64_Half e_ehsize;           ///< Size of this ELF header (64)
    Elf64_Half e_phentsize;        ///< Size of one program header entry (56)
    Elf64_Half e_phnum;            ///< Number of program headers
    Elf64_Half e_shentsize;        ///< Size of one section header entry
    Elf64_Half e_shnum;            ///< Number of section headers
    Elf64_Half e_shstrndx;         ///< Section name string table index
};
#pragma pack(pop)
static_assert(sizeof(Elf64_Ehdr) == 64, "Elf64_Ehdr must be exactly 64 bytes");

// -----------------------------------------------------------------------------
// ELF64 Program Header (Segment Descriptor) — 56 bytes total
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct Elf64_Phdr {
    Elf64_Word  p_type;    ///< Segment type (PT_LOAD, PT_SCE_DYNLIBDATA, etc.)
    Elf64_Word  p_flags;   ///< Segment flags (PF_R | PF_W | PF_X)
    Elf64_Off   p_offset;  ///< Offset of segment data in file
    Elf64_Addr  p_vaddr;   ///< Virtual address of segment in memory
    Elf64_Addr  p_paddr;   ///< Physical address (unused on PS5)
    Elf64_Xword p_filesz;  ///< Size of segment in file (can be 0 for BSS)
    Elf64_Xword p_memsz;   ///< Size of segment in memory
    Elf64_Xword p_align;   ///< Alignment (must be power of 2)
};
#pragma pack(pop)
static_assert(sizeof(Elf64_Phdr) == 56, "Elf64_Phdr must be exactly 56 bytes");

// -----------------------------------------------------------------------------
// ELF64 Dynamic Entry — 16 bytes
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct Elf64_Dyn {
    Elf64_Sxword d_tag;  ///< Dynamic tag (DT_NEEDED, DT_STRTAB, etc.)
    Elf64_Xword  d_val;  ///< Value (address or integer, depending on tag)
};
#pragma pack(pop)
static_assert(sizeof(Elf64_Dyn) == 16, "Elf64_Dyn must be exactly 16 bytes");

// -----------------------------------------------------------------------------
// ELF64 Symbol Table Entry — 24 bytes
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct Elf64_Sym {
    Elf64_Word  st_name;   ///< Symbol name (offset into string table)
    uint8_t     st_info;   ///< Type (low 4 bits) + binding (high 4 bits)
    uint8_t     st_other;  ///< Symbol visibility
    Elf64_Half  st_shndx;  ///< Section index (SHN_UNDEF = imported)
    Elf64_Addr  st_value;  ///< Symbol value (address or offset)
    Elf64_Xword st_size;   ///< Symbol size in bytes
};
#pragma pack(pop)
static_assert(sizeof(Elf64_Sym) == 24, "Elf64_Sym must be exactly 24 bytes");

// Symbol type/binding extraction macros
#define ELF64_ST_BIND(i)   (uint8_t)((i) >> 4)
#define ELF64_ST_TYPE(i)   (uint8_t)((i) & 0xF)

// Symbol binding values
constexpr uint8_t STB_LOCAL  = 0;
constexpr uint8_t STB_GLOBAL = 1;
constexpr uint8_t STB_WEAK   = 2;

// Symbol type values
constexpr uint8_t STT_NOTYPE  = 0;
constexpr uint8_t STT_OBJECT  = 1;
constexpr uint8_t STT_FUNC    = 2;
constexpr uint8_t STT_SECTION = 3;
constexpr uint8_t STT_FILE    = 4;

// Special section indices
constexpr Elf64_Half SHN_UNDEF  = 0;      ///< Undefined — this symbol is imported
constexpr Elf64_Half SHN_ABS    = 0xFFF1; ///< Absolute value
constexpr Elf64_Half SHN_COMMON = 0xFFF2; ///< Common block

// -----------------------------------------------------------------------------
// Program Header Types (p_type)
// -----------------------------------------------------------------------------
// Standard types
constexpr Elf64_Word PT_NULL         = 0;           ///< Unused entry
constexpr Elf64_Word PT_LOAD         = 1;           ///< Loadable segment (code/data)
constexpr Elf64_Word PT_DYNAMIC      = 2;           ///< Dynamic linking info
constexpr Elf64_Word PT_INTERP       = 3;           ///< Program interpreter path
constexpr Elf64_Word PT_NOTE         = 4;           ///< Auxiliary information
constexpr Elf64_Word PT_PHDR         = 6;           ///< Program header table itself
constexpr Elf64_Word PT_TLS          = 7;           ///< Thread-local storage
constexpr Elf64_Word PT_GNU_EH_FRAME = 0x6474E550;  ///< Exception handling
constexpr Elf64_Word PT_GNU_STACK    = 0x6474E551;  ///< Stack permissions
constexpr Elf64_Word PT_GNU_RELRO    = 0x6474E552;  ///< Read-only after relocation

// PS5-specific segment types (Sony OS-specific range 0x61000000+)
constexpr Elf64_Word PT_SCE_DYNLIBDATA  = 0x61000000; ///< PS5 dynamic library data
                                                        ///  (symbols, strings, relocations)
constexpr Elf64_Word PT_SCE_PROCPARAM   = 0x61000001; ///< Process parameters
constexpr Elf64_Word PT_SCE_MODULEPARAM = 0x61000002; ///< Module parameters
constexpr Elf64_Word PT_SCE_RELRO       = 0x61000010; ///< Read-only after relocation (PS5)
constexpr Elf64_Word PT_SCE_COMMENT     = 0x6FFFFF00; ///< Comment / metadata
constexpr Elf64_Word PT_SCE_LIBVERSION  = 0x6FFFFF01; ///< Library version info

// Program header flags (p_flags) — bitmask
constexpr Elf64_Word PF_X = 0x1;  ///< Execute permission
constexpr Elf64_Word PF_W = 0x2;  ///< Write permission
constexpr Elf64_Word PF_R = 0x4;  ///< Read permission

// -----------------------------------------------------------------------------
// Standard Dynamic Tags (d_tag in Elf64_Dyn)
// -----------------------------------------------------------------------------
constexpr Elf64_Sxword DT_NULL     = 0;   ///< End of dynamic array
constexpr Elf64_Sxword DT_NEEDED   = 1;   ///< Name of required library
constexpr Elf64_Sxword DT_PLTRELSZ = 2;   ///< Size of PLT relocs
constexpr Elf64_Sxword DT_PLTGOT   = 3;   ///< PLT/GOT address
constexpr Elf64_Sxword DT_HASH     = 4;   ///< Symbol hash table address
constexpr Elf64_Sxword DT_STRTAB   = 5;   ///< String table address
constexpr Elf64_Sxword DT_SYMTAB   = 6;   ///< Symbol table address
constexpr Elf64_Sxword DT_RELA     = 7;   ///< Relocation table (with addends)
constexpr Elf64_Sxword DT_RELASZ   = 8;   ///< Size of RELA table
constexpr Elf64_Sxword DT_RELAENT  = 9;   ///< Size of one RELA entry
constexpr Elf64_Sxword DT_STRSZ    = 10;  ///< Size of string table
constexpr Elf64_Sxword DT_SYMENT   = 11;  ///< Size of one symbol entry
constexpr Elf64_Sxword DT_INIT     = 12;  ///< Init function address
constexpr Elf64_Sxword DT_FINI     = 13;  ///< Fini function address
constexpr Elf64_Sxword DT_SONAME   = 14;  ///< Shared object name
constexpr Elf64_Sxword DT_JMPREL   = 23;  ///< PLT relocation table
constexpr Elf64_Sxword DT_PLTREL   = 20;  ///< Type of PLT relocs

// -----------------------------------------------------------------------------
// PS5/SCE-Specific Dynamic Tags
// Used inside the PT_SCE_DYNLIBDATA segment (Sony's custom dynamic section).
// These are offsets/sizes relative to the START of the PT_SCE_DYNLIBDATA segment,
// NOT virtual addresses — this differs from the standard dynamic section.
// -----------------------------------------------------------------------------
constexpr Elf64_Sxword DT_SCE_IDTABENTSZ   = 0x61000005; ///< ID table entry size
constexpr Elf64_Sxword DT_SCE_FINGERPRINT  = 0x61000007; ///< Binary fingerprint
constexpr Elf64_Sxword DT_SCE_FILENAME     = 0x61000009; ///< Original filename
constexpr Elf64_Sxword DT_SCE_MODULE_INFO  = 0x6100000D; ///< Module name + version
constexpr Elf64_Sxword DT_SCE_HASH         = 0x61000025; ///< Symbol hash table offset
constexpr Elf64_Sxword DT_SCE_HASHSZ       = 0x6100003D; ///< Symbol hash table size
constexpr Elf64_Sxword DT_SCE_PLTGOT       = 0x61000027; ///< PLT/GOT offset
constexpr Elf64_Sxword DT_SCE_JMPREL       = 0x61000029; ///< PLT reloc table offset
constexpr Elf64_Sxword DT_SCE_PLTREL       = 0x6100002B; ///< PLT reloc type
constexpr Elf64_Sxword DT_SCE_PLTRELSZ     = 0x6100002D; ///< PLT reloc table size
constexpr Elf64_Sxword DT_SCE_RELA         = 0x6100002F; ///< RELA table offset
constexpr Elf64_Sxword DT_SCE_RELASZ       = 0x61000031; ///< RELA table size
constexpr Elf64_Sxword DT_SCE_RELAENT      = 0x61000033; ///< RELA entry size
constexpr Elf64_Sxword DT_SCE_STRTAB       = 0x61000035; ///< String table offset
constexpr Elf64_Sxword DT_SCE_STRSZ        = 0x61000037; ///< String table size
constexpr Elf64_Sxword DT_SCE_SYMTAB       = 0x61000039; ///< Symbol table offset
constexpr Elf64_Sxword DT_SCE_SYMENT       = 0x6100003B; ///< Symbol entry size (24)
constexpr Elf64_Sxword DT_SCE_SYMTABSZ     = 0x6100003F; ///< Symbol table total size

