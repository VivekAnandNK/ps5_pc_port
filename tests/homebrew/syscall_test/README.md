# Test 2: Syscall Coverage

**Goal:** Exercise the most common Orbis OS syscalls to validate our stubs.
If this passes, most real games will get past startup without crashing on missing stubs.

## Syscalls to Test

### Threading (`libSceKernel`)
```c
// Create a thread, join it, verify it ran
ScePthread thread;
scePthreadCreate(&thread, NULL, my_func, NULL, "test_thread");
scePthreadJoin(thread, NULL);
```
- [ ] `scePthreadCreate()`
- [ ] `scePthreadJoin()`
- [ ] `scePthreadMutexInit()` / `scePthreadMutexLock()` / `scePthreadMutexUnlock()`

### Memory (`libSceKernel`)
```c
// Allocate, write, read, free
void* mem = sceKernelMapDirectMemory(...);
sceKernelReleaseDirectMemory(...);
```
- [ ] `sceKernelAllocateMainDirectMemory()`
- [ ] `sceKernelMapDirectMemory()`
- [ ] `sceKernelReleaseDirectMemory()`

### File I/O (`libSceKernel`)
```c
int fd = sceKernelOpen("/hostapp/data/test.txt", O_RDONLY, 0);
sceKernelRead(fd, buf, sizeof(buf));
sceKernelClose(fd);
```
- [ ] `sceKernelOpen()`
- [ ] `sceKernelRead()`
- [ ] `sceKernelClose()`

### Time (`libSceKernel`)
- [ ] `sceKernelGetProcessTime()`
- [ ] `sceKernelUsleep()`

## Pass Criteria
All syscalls return expected values. No `UNIMPLEMENTED STUB` errors in log.
