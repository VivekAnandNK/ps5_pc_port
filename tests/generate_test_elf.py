#!/usr/bin/env python3
"""
Generate a synthetic valid PS5 ELF file for testing the ELF parser.
Includes:
- ELF64 header with OS/ABI = 0x09 (ELFOSABI_SCE) and e_type = 0xFE00 (ET_SCE_EXEC)
- PT_LOAD segment (code/data)
- PT_SCE_PROCPARAM segment
- PT_SCE_DYNLIBDATA segment with DT_SCE dynamic entries, imported library string, and symbol table.
"""

import struct
import sys
from pathlib import Path

# Constants matching ps5_elf_types.h
EI_MAG = b"\x7fELF"
EI_CLASS64 = 2
EI_DATA2LSB = 1
EI_VERSION_CURR = 1
ELFOSABI_SCE = 0x09

ET_SCE_EXEC = 0xFE00
EM_X86_64 = 62
EV_CURRENT = 1

PT_LOAD = 1
PT_SCE_DYNLIBDATA = 0x61000000
PT_SCE_PROCPARAM = 0x61000001

PF_X = 1
PF_W = 2
PF_R = 4

DT_NULL = 0
DT_NEEDED = 1
DT_SCE_STRTAB = 0x61000035
DT_SCE_STRSZ = 0x61000037
DT_SCE_SYMTAB = 0x61000039
DT_SCE_SYMENT = 0x6100003B
DT_SCE_SYMTABSZ = 0x6100003F

def generate_ps5_elf(output_path: str):
    # Layout plan:
    # 0x0000: Elf64_Ehdr (64 bytes)
    # 0x0040: 3x Elf64_Phdr (3 * 56 = 168 bytes, ends at 0x00E8)
    # 0x0100: Code segment (PT_LOAD, 128 bytes)
    # 0x0200: ProcParam segment (PT_SCE_PROCPARAM, 64 bytes)
    # 0x0300: SCE DynLibData segment (PT_SCE_DYNLIBDATA)

    ehdr_size = 64
    phdr_size = 56
    num_phdrs = 3

    ph_offset = 64
    load_offset = 0x100
    load_vaddr = 0x400000
    load_filesz = 0x80
    load_memsz = 0x80

    param_offset = 0x200
    param_vaddr = 0x401000
    param_sz = 0x40

    dynlib_offset = 0x300
    dynlib_vaddr = 0x402000

    # Build PT_SCE_DYNLIBDATA content:
    # 1. Dynamic table (Elf64_Dyn: d_tag (8 bytes), d_val (8 bytes))
    # 2. String table (strtab)
    # 3. Symbol table (symtab, Elf64_Sym = 24 bytes)

    # String table
    strtab = b"\x00libSceKernel.sprx\x00sceKernelWrite\x00main\x00"
    strtab_offset_in_seg = 0x100 # relative to start of dynlibdata
    strtab_sz = len(strtab)

    lib_str_offset = 1 # index of "libSceKernel.sprx"
    write_str_offset = 1 + len("libSceKernel.sprx\x00")
    main_str_offset = write_str_offset + len("sceKernelWrite\x00")

    # Symbol table:
    # Elf64_Sym:
    # uint32 st_name
    # uint8  st_info (bind << 4 | type)
    # uint8  st_other
    # uint16 st_shndx (0 = SHN_UNDEF, imported)
    # uint64 st_value
    # uint64 st_size
    symtab_offset_in_seg = 0x180
    sym_entry_sz = 24

    # Sym 0: Null
    sym0 = struct.pack("<IBBHQQ", 0, 0, 0, 0, 0, 0)
    # Sym 1: sceKernelWrite (imported function: bind GLOBAL=1, type FUNC=2 -> 0x12, shndx=0 SHN_UNDEF)
    sym1 = struct.pack("<IBBHQQ", write_str_offset, (1 << 4) | 2, 0, 0, 0, 0)
    # Sym 2: main (global function defined at 0x400000, size 32)
    sym2 = struct.pack("<IBBHQQ", main_str_offset, (1 << 4) | 2, 0, 1, load_vaddr, 32)

    symtab = sym0 + sym1 + sym2
    symtab_sz = len(symtab)

    # Dyn entries:
    dyn_entries = [
        (DT_SCE_STRTAB, strtab_offset_in_seg),
        (DT_SCE_STRSZ, strtab_sz),
        (DT_SCE_SYMTAB, symtab_offset_in_seg),
        (DT_SCE_SYMENT, sym_entry_sz),
        (DT_SCE_SYMTABSZ, symtab_sz),
        (DT_NEEDED, lib_str_offset),
        (DT_NULL, 0)
    ]

    dyn_table = b"".join(struct.pack("<qq", tag, val) for tag, val in dyn_entries)

    # Assemble dynlib segment
    dynlib_data = bytearray(0x200)
    dynlib_data[0:len(dyn_table)] = dyn_table
    dynlib_data[strtab_offset_in_seg:strtab_offset_in_seg + strtab_sz] = strtab
    dynlib_data[symtab_offset_in_seg:symtab_offset_in_seg + symtab_sz] = symtab
    dynlib_sz = len(dynlib_data)

    # Create total file buffer
    file_size = dynlib_offset + dynlib_sz
    buf = bytearray(file_size)

    # Write ELF Header
    e_ident = bytearray(16)
    e_ident[0:4] = EI_MAG
    e_ident[4] = EI_CLASS64
    e_ident[5] = EI_DATA2LSB
    e_ident[6] = EI_VERSION_CURR
    e_ident[7] = ELFOSABI_SCE

    ehdr = struct.pack(
        "<16sHHIQQQIHHHHHH",
        bytes(e_ident),
        ET_SCE_EXEC,
        EM_X86_64,
        EV_CURRENT,
        load_vaddr,         # e_entry
        ph_offset,          # e_phoff
        0,                  # e_shoff
        0,                  # e_flags
        ehdr_size,          # e_ehsize
        phdr_size,          # e_phentsize
        num_phdrs,          # e_phnum
        0,                  # e_shentsize
        0,                  # e_shnum
        0                   # e_shstrndx
    )
    buf[0:64] = ehdr

    # Write Phdrs
    # 0: PT_LOAD (RX)
    phdr0 = struct.pack("<IIQQQQQQ", PT_LOAD, PF_R | PF_X, load_offset, load_vaddr, 0, load_filesz, load_memsz, 0x1000)
    # 1: PT_SCE_PROCPARAM (R)
    phdr1 = struct.pack("<IIQQQQQQ", PT_SCE_PROCPARAM, PF_R, param_offset, param_vaddr, 0, param_sz, param_sz, 8)
    # 2: PT_SCE_DYNLIBDATA (R)
    phdr2 = struct.pack("<IIQQQQQQ", PT_SCE_DYNLIBDATA, PF_R, dynlib_offset, dynlib_vaddr, 0, dynlib_sz, dynlib_sz, 0x10)

    buf[ph_offset:ph_offset + phdr_size] = phdr0
    buf[ph_offset + phdr_size:ph_offset + 2 * phdr_size] = phdr1
    buf[ph_offset + 2 * phdr_size:ph_offset + 3 * phdr_size] = phdr2

    # Dummy code for PT_LOAD: x86-64 nop sled + ret
    buf[load_offset:load_offset + 4] = b"\x90\x90\x90\xc3"

    # PT_SCE_PROCPARAM dummy
    buf[param_offset:param_offset + 4] = b"\x40\x00\x00\x00"

    # Write dynlib segment
    buf[dynlib_offset:dynlib_offset + dynlib_sz] = dynlib_data

    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "wb") as f:
        f.write(buf)

    print(f"Generated synthetic PS5 ELF at: {output_path} ({len(buf)} bytes)")

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "tests/homebrew/hello_world/test_sample.elf"
    generate_ps5_elf(out)
