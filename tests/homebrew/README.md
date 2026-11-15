# Homebrew Test Suite

Test binaries used to validate the translator pipeline **without any commercial game files**.
All sources here are open-source, freely distributed, or written by us.

## Test Progression (run in this order)

```
hello_world/        ← Can we load & execute a PS5 ELF at all?
     │
     ▼
syscall_test/       ← Do our Orbis OS stubs respond correctly?
     │
     ▼
memory_test/        ← Does our unified memory emulation work?
     │
     ▼
shader_test/        ← Can we translate Oberon GPU shaders → Vulkan?
```

## Sources

| Source | URL | What to grab |
|---|---|---|
| ps5-payload-sdk | https://github.com/john-tornblom/ps5-payload-sdk | Toolchain to compile PS5 ELFs on PC |
| ps5-playground | https://github.com/john-tornblom/ps5-playground | Small real PS5 payloads to test with |
| PS5 Homebrew Store | https://github.com/LightningMods/PS5-Store | Homebrew apps (real PS5 binaries) |
| shadPS4 test suite | https://github.com/shadps4-emu/shadPS4 | GPU + syscall test ROMs |

## Setup (Windows)

```bash
# Clone the PS5 payload SDK (gives us a cross-compiler for PS5 ELF format)
git clone https://github.com/john-tornblom/ps5-payload-sdk

# Clone test payloads
git clone https://github.com/john-tornblom/ps5-playground

# Build a test binary (from ps5-playground)
cd ps5-playground/example-hello
make
# → outputs a .elf file in PS5 format — feed this into our elf_parser
```

