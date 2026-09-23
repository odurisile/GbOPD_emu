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

For example, from the project folder in PowerShell:

```powershell
.\build\Debug\gba.exe .\rom\PE.gba
```

The executable loads the supplied ROM, resets the CPU, and executes 20
instructions using the current debug loop. It reports unsupported instructions
and loading failures with a nonzero exit code. This is a CPU smoke run; graphics,
BIOS services, and full game execution are not yet implemented. Use `--help` for
usage, and quote ROM paths that contain spaces.

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
