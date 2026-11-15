# Contributing to ps5_pc_port

Thank you for your interest in contributing! This is an open research project — all skill levels welcome.

---

## 📋 Before You Start

- Read the [README.md](./README.md) fully — especially the architecture diagram and "What to build next" section
- Check [open issues](../../issues) to avoid duplicate work
- For large changes, **open an issue first** to discuss your approach before writing code

---

## 🌿 Branch & Commit Rules

### Branch naming
```
feature/elf-parser-sections
fix/syscall-kernel-thread-crash
docs/shader-pipeline-notes
test/hello-world-homebrew
refactor/memory-manager-allocator
```

| Prefix | Use for |
|---|---|
| `feature/` | New functionality |
| `fix/` | Bug fixes |
| `docs/` | Documentation only |
| `test/` | Test additions |
| `refactor/` | Code cleanup, no behavior change |
| `chore/` | Build system, CI, tooling |

### Commit message format (Conventional Commits)
```
<type>(<scope>): <short summary>

[optional body]

[optional footer]
```

**Types:** `feat`, `fix`, `docs`, `test`, `refactor`, `chore`, `perf`

**Scopes:** `elf-parser`, `shader-compiler`, `syscall-stubs`, `memory-manager`, `vulkan-backend`, `dynamic-jit`, `docs`, `ci`

**Examples:**
```
feat(elf-parser): parse PS5 program headers and section table
fix(syscall-stubs): sceKernelWrite returns correct byte count
docs(shader-compiler): add Oberon ISA register mapping table
test(hello-world): add expected output for sceKernelExit stub
perf(vulkan-backend): cache pipeline state objects by hash
```

### Rules
- ✅ One logical change per commit
- ✅ Present tense, imperative mood ("add", not "added" or "adds")
- ✅ Summary line ≤ 72 characters
- ❌ No "WIP", "fix stuff", "misc changes" commit messages
- ❌ No committing directly to `main`

---

## 🔀 Pull Request Process

1. Fork the repo and create your branch from `main`
2. Make sure your code **builds cleanly** before opening a PR
3. Add or update tests in `tests/` for any logic changes
4. Fill out the PR template fully
5. PRs require **at least 1 review** before merging
6. Squash commits if the branch history is messy before merge

---

## 🏗️ Code Style

**C++ (if used):**
- C++17 or later
- `clang-format` with the project's `.clang-format` (coming soon)
- Prefer `std::` types over raw C equivalents
- No raw `new`/`delete` — use smart pointers

**Rust (if used):**
- `rustfmt` — format before every commit (`cargo fmt`)
- `clippy` warnings must be clean (`cargo clippy`)
- Safe Rust preferred; `unsafe` blocks must have a comment explaining why

**General:**
- No hardcoded file paths — use config or arguments
- No dead code committed (comment it out or delete it)
- Every public function/struct needs a doc comment

---

## ⚖️ Legal Rules — Non-Negotiable

> These rules exist to keep this project legal and protect all contributors.

- ❌ **Never** commit copyrighted PS5 game binaries (`.pkg`, `.elf` from commercial titles)
- ❌ **Never** commit decryption keys, firmware dumps, or anything extracted from a PS5 console
- ❌ **Never** copy code from Sony's SDKs, firmware, or any proprietary source
- ✅ **Always** implement PS5 interfaces clean-room (from observed behavior, public docs, reverse engineering write-ups)
- ✅ PRs containing any of the above will be **immediately closed** without review

---

## 🐛 Reporting Issues

Use the GitHub issue templates:
- **Bug report** — something is broken
- **Feature request** — new module or capability
- **Research note** — technical findings worth documenting

---

## 💬 Questions?

Open a [Discussion](../../discussions) rather than an issue for general questions, ideas, or architecture debates.

