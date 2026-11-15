cmake_minimum_required(VERSION 3.16)

project(ps5_elf_parser
    VERSION 0.1.0
    DESCRIPTION "PS5 ELF binary parser -- Phase 1"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Source files ──────────────────────────────────────────────────────────────
add_executable(ps5_elf_parser
    src/main.cpp
    src/elf_parser.cpp
)

target_include_directories(ps5_elf_parser PRIVATE include)

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(ps5_elf_parser PRIVATE /W4 /WX)
else()
    target_compile_options(ps5_elf_parser PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

# ── Output binary next to source for convenience ──────────────────────────────
set_target_properties(ps5_elf_parser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS ps5_elf_parser DESTINATION bin)

message(STATUS "")
message(STATUS "  PS5 ELF Parser")
message(STATUS "  Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:    ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Output:      ${CMAKE_BINARY_DIR}/bin/ps5_elf_parser")
message(STATUS "")

