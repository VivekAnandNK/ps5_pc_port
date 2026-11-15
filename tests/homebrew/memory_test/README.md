# Test 3: Memory Model

**Goal:** Verify our unified memory emulation correctly handles PS5 memory layout.

## Background

PS5 uses a **Unified Memory Architecture (UMA)**:
- Single 16 GB GDDR6 pool shared by CPU and GPU
- No copy overhead between CPU and GPU memory
- Games allocate from one flat address space

PC has **split memory**:
- System RAM (DDR4/DDR5) for CPU
- VRAM (GDDR6) for GPU, accessed via PCIe
- Must explicitly copy between them (unless using ReBAR/SAM)

## What We Emulate

```
PS5 Unified Pool (16GB)          PC Emulated Layout
┌─────────────────────┐          ┌─────────────────────┐
│   CPU data (top)    │  ──────► │  System RAM region   │
├─────────────────────┤          ├─────────────────────┤
│   Shared region     │  ──────► │  Pinned + mapped     │
├─────────────────────┤          ├─────────────────────┤
│   GPU data (bottom) │  ──────► │  VRAM region (Vulkan)│
└─────────────────────┘          └─────────────────────┘
```

## Tests

- [ ] Allocate 1 GB block → verify address is in PS5 range (0x100000000+)
- [ ] Write from "CPU side" → read from "GPU side" → data matches
- [ ] Allocate at exact PS5 address hint → respect the hint or remap cleanly
- [ ] Exhaust pool → verify correct error code (`SCE_KERNEL_ERROR_ENOMEM`)
- [ ] Release memory → verify it returns to pool correctly

## Pass Criteria
No memory corruption. Addresses fall in expected PS5 virtual ranges.
GPU-side reads of CPU-written data are consistent.
