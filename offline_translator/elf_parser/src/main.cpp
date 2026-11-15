// =============================================================================
// main.cpp — CLI entry point for the PS5 ELF Parser
//
// Usage:
//   ps5_elf_parser <path/to/file.elf>
//
// Examples:
//   ps5_elf_parser tests/homebrew/hello_world.elf
//   ps5_elf_parser /path/to/ps5-playground/example-hello/eboot.elf
// =============================================================================

#include "elf_parser.h"

#include <iostream>
#include <string>

// Enable UTF-8 output on Windows (for the box-drawing characters in dump())
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {

#ifdef _WIN32
    // Allow UTF-8 output in Windows terminal
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc != 2) {
        std::cout << "\n";
        std::cout << "PS5 ELF Parser v0.1 -- Phase 1\n";
        std::cout << "-------------------------------\n";
        std::cout << "Parses PS5 ELF binaries and dumps their structure.\n\n";
        std::cout << "Usage:\n";
        std::cout << "  ps5_elf_parser <path/to/file.elf>\n\n";
        std::cout << "Examples:\n";
        std::cout << "  ps5_elf_parser eboot.elf\n";
        std::cout << "  ps5_elf_parser tests/homebrew/hello_world.elf\n\n";
        std::cout << "To get PS5 test binaries:\n";
        std::cout << "  git clone https://github.com/john-tornblom/ps5-playground\n";
        std::cout << "  (then build with ps5-payload-sdk)\n\n";
        return 1;
    }

    std::string path = argv[1];

    std::cout << "Parsing: " << path << "\n";

    ps5::ElfParser parser;

    if (!parser.parse(path)) {
        std::cerr << "\n[ERROR] " << parser.error() << "\n\n";
        return 1;
    }

    parser.dump();

    return 0;
}

