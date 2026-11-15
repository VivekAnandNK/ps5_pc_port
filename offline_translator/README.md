# Offline Translator

Runs **once per game**, before launch. Produces a `cache/` of pre-translated assets so the runtime layer has minimal work to do.

## Pipeline

```
game.pkg
   │
   ├─► decryptor/      → raw ELF + asset files
   ├─► elf_parser/     → code segments, symbol table, imports
   ├─► shader_compiler/→ cache/shaders/*.spv  (SPIR-V)
   ├─► syscall_stubber/→ list of all Orbis syscalls the game uses
   └─► code_translator/→ cache/code_segments/*.bin (native x86)
```

## Modules

| Module | Job |
|---|---|
| `decryptor` | Strip PS5 `.pkg` encryption, extract raw ELF |
| `elf_parser` | Walk the ELF, identify static vs dynamic code sections |
| `shader_compiler` | Decode Oberon GPU ISA → emit SPIR-V for Vulkan |
| `syscall_stubber` | Scan imports, generate a stub map for the runtime |
| `code_translator` | Rewrite static x86 sections (fix PS5-specific instructions, calling conventions) |

