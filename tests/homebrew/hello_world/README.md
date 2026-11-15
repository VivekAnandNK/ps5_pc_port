# Test 1: Hello World

**Goal:** Verify the ELF parser can load a minimal PS5 binary and the runtime
can execute a single syscall (`sceKernelWrite` → stdout).

## What this tests
- [ ] ELF header parsing (PS5 magic bytes, section layout)
- [ ] Program segment loading into emulated memory
- [ ] Import resolution (`libSceKernel.sprx` stub lookup)
- [ ] `sceKernelWrite()` syscall stub → writes "Hello from PS5!" to stdout
- [ ] Clean process exit via `sceKernelExit()`

## Source

Write this using `ps5-payload-sdk`. Equivalent C code:

```c
// hello_orbis.c
// Compile with: ps5-payload-sdk toolchain
// Target: PS5 ELF (little-endian x86-64, Orbis ABI)

#include <orbis/libkernel.h>

int main(void) {
    // sceKernelWrite = Orbis equivalent of write()
    // fd 1 = stdout
    sceKernelWrite(1, "Hello from PS5!\n", 16);
    return 0;
}
```

## Expected Output

```
[ELF Parser]    Magic OK: 7f 45 4c 46
[ELF Parser]    Type: ET_SCE_EXEC (PS5 executable)
[ELF Parser]    Entry point: 0x401000
[ELF Parser]    Imports: libSceKernel.sprx -> sceKernelWrite, sceKernelExit
[Syscall Stub]  sceKernelWrite(1, "Hello from PS5!\n", 16)
Hello from PS5!
[Syscall Stub]  sceKernelExit(0)
[Launcher]      Process exited cleanly.
```

## Pass Criteria
Output contains `Hello from PS5!` with no crashes or unimplemented syscall errors.
