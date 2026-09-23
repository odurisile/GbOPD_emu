# CPU regression tests

Build and run the tests with:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The dependency-free test executable runs small instruction sequences from EWRAM
through the real CPU and memory bus. Checks cover ARM conditions, arithmetic
carry/borrow and overflow, shifts at zero/32/greater-than-32, PC pipeline offsets,
branches and links, ARM/Thumb exchange, aligned and unaligned memory transfers,
signed loads, multiply and long multiply, stack transfers, Thumb arithmetic and
literal loads, status flag access, reset, and unsupported-instruction diagnostics.
Assertions remain active in Release builds.

## CPU implementation scope

`CPU::step()` executes one instruction. `registers()[15]` / `pc()` is the next
fetch address; instruction operands see the architectural PC offset. `cpsr()`
exposes status for inspection. `reset()` retains the direct-ROM entry at
`0x08000000`; `reset(address)` allows RAM test programs, with bit zero selecting
Thumb state. General registers start at zero, so programs must initialize SP.

Implemented ARM families:

- Conditional data processing and comparisons, immediate and register shifts.
- B, BL, BX; MUL/MLA and signed/unsigned long multiplies with accumulation.
- Word/byte/halfword transfers, signed loads, SWP/SWPB, block transfers.
- MRS CPSR and MSR CPSR flag/control writes, with User-mode restrictions and
  banked SP/LR registers plus FIQ r8-r12. User and System share a register bank.

Implemented Thumb families:

- Shifts, arithmetic, logic, comparisons, high-register operations and BX.
- PC/SP-relative, immediate-offset and register-offset memory transfers.
- Address generation, SP adjustment, PUSH/POP and multiple transfers.
- Conditional/unconditional branches and the two-instruction BL sequence.

This is a functional interpreter, not a complete ARM7TDMI implementation.
BIOS/SWI services, IRQ/FIQ delivery, SPSRs, exception return,
user-bank block transfers, empty block-transfer lists,
and cycle/bus timing are not implemented. Unsupported instructions throw
`std::runtime_error` with the instruction and its address rather than silently
acting as no-ops. The fetch PC has already advanced when an error is raised.
Unpredictable/reserved instruction encodings are not exhaustively validated.

The memory bus still lacks BIOS and IO support. `main.cpp` loads the ROM into
the emulator and invokes its current 20-instruction debug run. CPU tests invoke
the CPU directly; CLI smoke tests also verify ROM loading, CPU execution, and
error reporting through the executable. These do not establish game compatibility.

Encoding reference: [Arm ARM7TDMI Technical Reference Manual](https://documentation-service.arm.com/static/5e8e1323fd977155116a3129).
