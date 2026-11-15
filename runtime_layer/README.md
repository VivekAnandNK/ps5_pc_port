# Runtime Layer

Active **the entire time the game is running**. Loads the pre-translated cache and handles the small portion of work that couldn't be done offline.

## Responsibilities

| Module | Job |
|---|---|
| `syscall_stubs` | Intercept Orbis OS calls → forward to Win32/Linux equivalents |
| `dynamic_code` | JIT-compile code the game generates at runtime (can't be pre-translated) |
| `vulkan_backend` | Load pre-compiled SPIR-V shaders, submit draw calls via Vulkan |
| `memory_manager` | Emulate PS5's unified 16GB GDDR6 pool across split PC RAM + VRAM |

## Design Goal

Runtime overhead should be **near zero** for static code paths.
Only dynamic/JIT paths carry overhead — typically < 5% of total execution.

