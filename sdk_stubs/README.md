# SDK Stubs

Clean-room reimplementations of PS5 SDK libraries.

These are **not** copies of Sony's code. Each function is independently
reimplemented by studying publicly known behavior (reverse engineering,
documentation, community research).

## Key Libraries to Stub

| PS5 Library | Purpose | PC Equivalent |
|---|---|---|
| `libSceKernel` | Threading, memory, IPC | pthreads / Win32 threads |
| `libSceGnm` | Low-level GPU submission | Vulkan |
| `libSceAudioOut` | Audio output | PortAudio / DirectSound |
| `libSceGamepad` | Controller input | SDL2 / XInput |
| `libSceSaveData` | Save file management | Local filesystem |
| `libSceNet` | Networking | BSD sockets / WinSock |
| `libSceNpTrophy` | Trophy system | Stub (no-op) |

## Implementation Status

- [ ] libSceKernel — in progress
- [ ] libSceGnm — in progress  
- [ ] libSceAudioOut — not started
- [ ] libSceGamepad — not started
- [ ] libSceSaveData — not started
- [ ] libSceNet — not started
- [ ] libSceNpTrophy — not started

