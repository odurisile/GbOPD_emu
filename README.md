# GBA Emulator Skeleton

A minimal C++20 architecture for a Game Boy Advance emulator.

## Current structure

- `Emulator` — owns all hardware components and runs the main loop
- `CPU` — ARM7TDMI register state and instruction stepping
- `Memory` — GBA memory map / bus
- `Cartridge` — ROM loading
- `PPU` — video subsystem placeholder
- `Timers` — timer subsystem placeholder

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/gba path/to/game.gba
```

On Windows with Visual Studio generators, the executable may be under:

```text
build/Debug/gba.exe
```

## Recommended implementation order

1. ROM loading
2. Memory map
3. CPU registers + CPSR
4. ARM instruction fetch/decode
5. ARM data-processing instructions
6. Branches
7. Loads/stores
8. THUMB instruction decoding
9. BIOS/SWI handling
10. PPU timing + framebuffer
11. Interrupts
12. Timers
13. Keypad
14. DMA
15. Audio
16. Save memory
