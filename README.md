# 🎮 PS5 → PC Port (Hybrid Translation Layer)

> **⚡ AI CONTEXT BRIEF** — Read this first if you're an AI assistant helping with this project.

---

## 🧠 What This Project Is

An **experimental research project** to run PS5 games natively on a PC using a **Hybrid AOT + Minimal Runtime** translation approach. No PS5 console required.

This is NOT a typical emulator. We are **not** emulating PS5 hardware in software. Instead we exploit the fact that the PS5 uses an **x86-64 CPU** (same as a PC) and translate/relink the game binary to run natively on Windows with Vulkan.

---

## 💡 The Core Insight

| Component | PS5 | PC | Gap |
|---|---|---|---|
| CPU ISA | x86-64 (Zen 2) | x86-64 | ✅ **None — same architecture** |
| GPU API | Sony `libSceGnm` (custom, low-level) | Vulkan | 🔴 Must translate shaders |
| OS / Kernel | Orbis OS (FreeBSD-derived, proprietary) | Windows | 🔴 Must stub syscalls |
| Memory | 16GB GDDR6 Unified (CPU+GPU share) | Split RAM + VRAM | 🟡 Must emulate layout |
| Storage I/O | Custom SSD + hardware Oodle decompressor | DirectStorage / software | 🟡 Emulatable |

Since the CPU ISA is identical, **we don't need to recompile or emulate CPU instructions**. The two hard problems are:
1. **GPU Shader Translation** — Oberon ISA (PS5 GPU) → SPIR-V → Vulkan
2. **Orbis OS Syscall Stubbing** — Reimplement PS5 OS APIs as clean-room Windows/Linux equivalents

---

## 🏗️ Architecture: Hybrid AOT + Minimal Runtime

We chose the **hybrid approach** because:
- Pure runtime translation = high CPU overhead + shader stutter every frame
- Pure static (AOT) translation to `.exe` = impossible due to dynamic code generation + encryption
- **Hybrid** = pre-translate everything possible offline, handle only dynamic code at runtime

```
┌─────────────────────────────────────────────────────────────────┐
│                OFFLINE PHASE (run once per game)                │
│                                                                 │
│  game.pkg ──► decryptor ──► elf_parser                          │
│                                  │                              │
│                    ┌─────────────┴─────────────┐               │
│                    ▼                           ▼               │
│           Static Code Sections         GPU Shaders (Oberon ISA) │
│                    │                           │               │
│           code_translator              shader_compiler          │
│           (PS5 x86 → PC x86)          (Oberon → SPIR-V)        │
│                    │                           │               │
│                    └─────────────┬─────────────┘               │
│                                  ▼                              │
│                     cache/  (pre-built assets)                  │
│               shaders/*.spv + code_segments/*.bin               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼ (game launch)
┌─────────────────────────────────────────────────────────────────┐
│               RUNTIME PHASE (while game is playing)             │
│                                                                 │
│  Load cached pre-translated code  ←── near-zero overhead        │
│  syscall_stubs: Orbis OS → Win32  ←── intercepted & forwarded   │
│  dynamic_code: JIT for runtime-generated code ←── small %       │
│  vulkan_backend: submit SPIR-V shaders to GPU ←── standard      │
│  memory_manager: emulate unified memory pool                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📂 Project Structure

```
ps5_pc_port/
│
├── offline_translator/          # Run ONCE before launching a game
│   ├── decryptor/               # Decrypt PS5 .pkg container format
│   ├── elf_parser/              # Parse PS5 ELF binary (find code, imports, shaders)
│   ├── shader_compiler/         # Oberon GPU ISA → SPIR-V (Vulkan-ready shaders)
│   ├── syscall_stubber/         # Scan binary → map all Orbis syscall imports
│   └── code_translator/         # Rewrite static PS5 x86 → PC-compatible x86
│
├── runtime_layer/               # Active only while game is running
│   ├── syscall_stubs/           # Clean-room Orbis OS API → Win32/Linux
│   ├── dynamic_code/            # JIT compiler for runtime-generated code
│   ├── vulkan_backend/          # Load SPIR-V cache → submit to Vulkan GPU
│   └── memory_manager/          # Emulate 16GB unified GDDR6 on split PC memory
│
├── sdk_stubs/                   # Clean-room PS5 SDK library reimplementations
│   │                            # (libSceKernel, libSceGnm, libSceAudioOut, etc.)
│
├── cache/                       # OUTPUT of offline_translator (not committed)
│   ├── shaders/                 # Pre-compiled SPIR-V shaders
│   └── code_segments/           # Pre-translated native x86 code
│
├── tools/                       # Dev utilities (disassemblers, packet dumpers, etc.)
├── docs/                        # Architecture notes and research
└── tests/
    ├── unit/
    └── integration/
```

---

## 🔧 Development Environment

**Primary machine: Windows laptop** (chosen deliberately)
- PS5 is x86-64 → matches Windows PC natively (no extra CPU translation layer)
- Vulkan support is native on Windows (no MoltenVK overhead like on macOS)
- All reference projects (shadPS4, RPCS3, Kyty) target Windows/Linux first
- RenderDoc + PIX available for GPU shader debugging

> ⚠️ **Mac users:** If your Mac is Apple Silicon (M1/M2/M3/M4), it's ARM-based. Running this project would add a 3rd translation layer (PS5 x86 → our translator → ARM via Rosetta → Vulkan via MoltenVK → Metal). Use the Windows machine instead.

**Language:** TBD (C++ is industry standard for emulators; Rust is a strong alternative)

---

## 🧪 Test Binaries — Where to Get PS5 ELFs (Legal)

**Do NOT use pirated game files.** Use these free, open-source sources instead:

| Source | URL | What it gives you |
|---|---|---|
| **ps5-payload-sdk** | https://github.com/john-tornblom/ps5-payload-sdk | Cross-compiler: build real PS5 ELFs on your PC |
| **ps5-playground** | https://github.com/john-tornblom/ps5-playground | Small test payloads in PS5 ELF format |
| **PS5 Homebrew Store** | https://github.com/LightningMods/PS5-Store | Full homebrew apps (real PS5 binaries, open source) |
| **shadPS4 test suite** | https://github.com/shadps4-emu/shadPS4 | GPU + syscall test ROMs for PS4/PS5 |

### Test progression (see `tests/homebrew/`)
```
Test 1: hello_world   → ELF loader + sceKernelWrite stub
Test 2: syscall_test  → threading, memory, file I/O stubs
Test 3: memory_test   → unified memory emulation
Test 4: shader_test   → Oberon GPU ISA → SPIR-V → Vulkan
```

---

## 📚 Key Reference Projects to Study

| Project | What to learn from it |
|---|---|
| [shadPS4](https://github.com/shadps4-emu/shadPS4) | **Best reference** — PS4 compat layer in C++, similar architecture to what we're building |
| [RPCS3](https://github.com/RPCS3/rpcs3) | Mature emulator — excellent patterns for syscall stubbing, shader cache, JIT |
| [Kyty / KytyPS5](https://github.com/InoriRus/Kyty) | Actual PS5 translation layer attempt |
| [Chiaki4deck](https://github.com/streetpea/chiaki4deck) | PS Remote Play protocol (Track A, shelved — requires physical PS5) |

### Key Specs / Docs to Read
- AMD RDNA 2 ISA Reference Manual — to understand Oberon GPU shader instructions
- SPIR-V Specification (Khronos Group) — our shader compilation target
- Vulkan spec — our graphics API
- FreeBSD kernel internals — Orbis OS is based on FreeBSD
- Mark Cerny PS5 Architecture GDC talk — hardware deep dive

---

## 🗺️ What To Build Next (Suggested Order)

1. **ELF Parser** (`offline_translator/elf_parser/`) — Entry point for everything. Can't translate shaders or syscalls until we can read the binary format.
2. **Syscall Stubber** (`offline_translator/syscall_stubber/`) — Scan imports, list all Orbis APIs the game calls. Needed to know what to implement.
3. **SDK Stubs skeleton** (`sdk_stubs/`) — Implement the most common syscalls: `libSceKernel` (threads, memory), `libSceSaveData`, `libSceGamepad`
4. **Shader Compiler** (`offline_translator/shader_compiler/`) — Hardest piece. Study shadPS4's shader recompiler as reference.
5. **Vulkan Backend** (`runtime_layer/vulkan_backend/`) — Load SPIR-V, create pipeline, submit draw calls.
6. **Memory Manager** (`runtime_layer/memory_manager/`) — Emulate unified memory layout.

---

## ⚖️ Legal Notes

> [!CAUTION]
> - Only use **legally owned** PS5 games or freely distributed **homebrew binaries**
> - All SDK reimplementations must be **clean-room** (no Sony code, reverse-engineered from observed behavior)
> - No Sony proprietary code is used or distributed in this repo
> - This project is purely for **research and educational purposes**

---

## 🤝 Session History (Mac Mini → Windows handoff)

**Session 1 (Mac Mini, Nov 15 2026):**
- Researched PS5 architecture and all 3 approaches (Remote Play, Translation Layer, Full Emulation)
- Decided: **Track B — Translation Layer** (Remote Play requires physical PS5)
- Decided: **Hybrid AOT + Minimal Runtime** approach (pure static `.exe` impossible due to dynamic code + encryption; pure runtime has high overhead)
- Decided: **Windows laptop** as dev machine (Apple Silicon Mac adds extra ARM translation layer)
- Scaffolded the full project structure
- Next step: pick a language (C++ or Rust) and start the ELF parser

---

*Last updated: Nov 15 2026 — Mac Mini session*
