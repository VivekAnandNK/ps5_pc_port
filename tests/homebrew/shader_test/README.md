# Test 4: Shader Translation

**Goal:** Take a real PS5 GPU shader (Oberon ISA binary), run it through our
shader_compiler, and verify the output SPIR-V produces correct results in Vulkan.

## Background

PS5 games submit shaders compiled for the "Oberon" GPU (custom RDNA 2).
These are raw binary blobs in GCN/RDNA2 instruction format with PS5-specific extensions.
We must decode → translate → emit SPIR-V → verify via Vulkan.

## Test Shaders (start simple, increase complexity)

### Shader 1: Pass-through (trivial)
```glsl
// Equivalent intent — input color passes straight to output
// Tests: basic SPIR-V emission, Vulkan pipeline creation
void main() {
    fragColor = vertexColor;
}
```
- [ ] Oberon binary → decoded instructions log
- [ ] SPIR-V emitted without errors
- [ ] `spirv-val` validation passes (use Khronos SPIRV-Tools)
- [ ] Vulkan pipeline creation succeeds
- [ ] Rendered output matches expected color (e.g. solid red quad)

### Shader 2: Texture sample
```glsl
// Tests: texture descriptor handling, sampler binding
void main() {
    fragColor = texture(uSampler, uv);
}
```
- [ ] Texture descriptor correctly mapped from PS5 T# format → Vulkan descriptor set
- [ ] Output image matches source texture

### Shader 3: Compute shader (async compute)
- [ ] PS5 compute dispatch → Vulkan `vkCmdDispatch`
- [ ] Shared memory (LDS on PS5) → Vulkan workgroup shared memory

## Tools Needed
- `spirv-val` — from [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools)
- `spirv-dis` — disassemble SPIR-V to human-readable form for debugging
- RenderDoc — capture and inspect Vulkan frames

## Pass Criteria
`spirv-val` reports no errors. Vulkan renders expected output for each shader.

