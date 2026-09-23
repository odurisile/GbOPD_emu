#include "gba/cartridge.hpp"
#include "gba/cpu.hpp"
#include "gba/memory.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace gba;
namespace {
void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
struct Machine {
    Cartridge cartridge;
    Memory memory{cartridge};
    CPU cpu{memory};
    Machine() { cpu.reset(0x02000000); }
    void arm(u32 ins) { memory.write32(cpu.pc(), ins); cpu.step(); }
    void thumb(u16 ins) { memory.write16(cpu.pc(), ins); cpu.step(); }
    void reg(unsigned r, u32 value) {
        memory.write32(cpu.pc() + 8, value);
        arm(0xe59f0000 | (r << 12)); // LDR r, [pc]
    }
    void enterThumb() { reg(12, 0x02001001); arm(0xe12fff1c); }
};
void arithmetic() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0x7fffffff); m->arm(0xe2901001); // ADDS r1,r0,#1
    check(m->cpu.registers()[1] == 0x80000000, "ADD result");
    check((m->cpu.cpsr() & 0xf0000000) == 0x90000000, "ADD signed overflow");
    m->reg(0, 0xffffffff); m->arm(0xe2901001);
    check((m->cpu.cpsr() & 0xf0000000) == 0x60000000, "ADD carry and zero");
    m->arm(0xe2b12000); // ADCS r2,r1,#0
    check(m->cpu.registers()[2] == 1, "ADC carry input");
    m->arm(0xe2513001); // SUBS r3,r1,#1
    check(m->cpu.registers()[3] == 0xffffffff && !(m->cpu.cpsr() & 0x20000000), "SUB borrow");
    m->arm(0xe2d14000); // SBCS r4,r1,#0
    check(m->cpu.registers()[4] == 0xffffffff, "SBC borrow input");
}
void conditions() {
    auto m = std::make_unique<Machine>();
    for (unsigned sample = 0; sample < 4; ++sample) {
        m->reg(0, sample == 0 ? 0 : sample == 1 ? 1 : sample == 2 ? 0x80000000 : 0x7fffffff);
        m->arm(0xe3500001); // CMP r0,#1
        const u32 f = m->cpu.cpsr();
        const bool n = (f >> 31) != 0, z = ((f >> 30) & 1) != 0;
        const bool c = ((f >> 29) & 1) != 0, v = ((f >> 28) & 1) != 0;
        const bool expected[] = {z,!z,c,!c,n,!n,v,!v,c&&!z,!c||z,n==v,n!=v,!z&&n==v,z||n!=v,true,false};
        for (unsigned cond = 0; cond < 16; ++cond) {
            m->reg(1, 0);
            m->arm((cond << 28) | 0x03a01001);
            check(m->cpu.registers()[1] == unsigned(expected[cond]), "ARM condition");
        }
    }
}
void shifts() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0x80000001);
    m->arm(0xe1b01020); // MOVS r1,r0,LSR #32
    check(m->cpu.registers()[1] == 0 && (m->cpu.cpsr() & 0x20000000), "LSR #32");
    m->arm(0xe1b01060); // RRX with carry set
    check(m->cpu.registers()[1] == 0xc0000000, "RRX");
    m->reg(2, 32); m->arm(0xe1b01210); // LSL r2
    check(m->cpu.registers()[1] == 0 && (m->cpu.cpsr() & 0x20000000), "register LSL 32");
    m->reg(2, 33); m->arm(0xe1b01210);
    check(m->cpu.registers()[1] == 0 && !(m->cpu.cpsr() & 0x20000000), "register LSL 33");
    m->reg(2, 255); m->arm(0xe1b01250); // ASR r2
    check(m->cpu.registers()[1] == 0xffffffff, "ASR large shift");
    m->reg(2, 0); m->arm(0xe1b01270); // ROR r2
    check(m->cpu.registers()[1] == 0x80000001 && (m->cpu.cpsr() & 0x20000000), "zero register shift preserves carry");
    m->arm(0xe3b01102); // MOVS r1,#0x80000000
    check(m->cpu.registers()[1] == 0x80000000 && (m->cpu.cpsr() & 0x20000000), "rotated immediate");
}
void branches() {
    auto m = std::make_unique<Machine>();
    u32 address = m->cpu.pc();
    m->arm(0xe1a0000f); check(m->cpu.registers()[0] == address + 8, "visible ARM PC");
    address = m->cpu.pc(); m->arm(0xeb000001);
    check(m->cpu.pc() == address + 12 && m->cpu.registers()[14] == address + 4, "BL target and link");
    address = m->cpu.pc(); m->arm(0xeafffffe);
    check(m->cpu.pc() == address, "backward branch");
    m->enterThumb(); check(m->cpu.pc() == 0x02001000 && (m->cpu.cpsr() & 32), "BX to Thumb");
    m->thumb(0x4678); check(m->cpu.registers()[0] == 0x02001004, "visible Thumb PC");
    m->thumb(0x4700); check(m->cpu.pc() == 0x02001004 && !(m->cpu.cpsr() & 32), "BX to ARM");
}
void transfers() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0x02002000); m->reg(1, 0x81223344);
    m->arm(0xe4801004); // STR r1,[r0],#4
    check(m->memory.read32(0x02002000) == 0x81223344 && m->cpu.registers()[0] == 0x02002004, "STR post increment");
    m->arm(0xe5102003); // LDR r2,[r0,#-3]
    check(m->cpu.registers()[2] == 0x44812233, "unaligned LDR rotation");
    m->arm(0xe5503001); check(m->cpu.registers()[3] == 0x81, "LDRB");
    m->arm(0xe15040d1); check(m->cpu.registers()[4] == 0xffffff81, "LDRSB negative");
    m->arm(0xe15050f2); check(m->cpu.registers()[5] == 0xffff8122, "LDRSH negative");
    m->arm(0xe14010b0); check(m->memory.read16(0x02002004) == 0x3344, "STRH");
    m->arm(0xe1006091); check(m->cpu.registers()[6] == 0x3344 && m->memory.read32(0x02002004) == 0x81223344, "SWP");
}
void multiply() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0xfffffffe); m->reg(1, 3);
    m->arm(0xe0120190); check(m->cpu.registers()[2] == 0xfffffffa, "MUL");
    m->arm(0xe0d32190); // SMULLS r2,r3,r0,r1
    check(m->cpu.registers()[2] == 0xfffffffa && m->cpu.registers()[3] == 0xffffffff, "SMULL");
    check((m->cpu.cpsr() & 0xc0000000) == 0x80000000, "long multiply flags");
    m->arm(0xe0832190); // UMULL r2,r3,r0,r1
    check(m->cpu.registers()[2] == 0xfffffffa && m->cpu.registers()[3] == 2, "UMULL");
}
void blockTransfers() {
    auto m = std::make_unique<Machine>();
    m->reg(13, 0x02003000); m->reg(0, 123); m->reg(1, 456);
    m->arm(0xe92d0003); // STMDB sp!,{r0,r1}
    check(m->cpu.registers()[13] == 0x02002ff8 && m->memory.read32(0x02002ffc) == 456, "ARM push");
    m->reg(0, 0); m->reg(1, 0); m->arm(0xe8bd0003);
    check(m->cpu.registers()[0] == 123 && m->cpu.registers()[1] == 456 && m->cpu.registers()[13] == 0x02003000, "ARM pop");
}
void thumbArithmeticAndStack() {
    auto m = std::make_unique<Machine>();
    m->reg(13, 0x02003000); m->enterThumb();
    m->thumb(0x2005); m->thumb(0x2103); m->thumb(0x1842); // r2=r0+r1
    check(m->cpu.registers()[2] == 8, "Thumb add registers");
    m->thumb(0x3a08); check(m->cpu.registers()[2] == 0 && (m->cpu.cpsr() & 0x40000000), "Thumb subtract immediate flags");
    m->thumb(0x4348); check(m->cpu.registers()[0] == 15, "Thumb multiply");
    m->thumb(0xb403); m->thumb(0x2000); m->thumb(0x2100); m->thumb(0xbc03);
    check(m->cpu.registers()[0] == 15 && m->cpu.registers()[1] == 3 && m->cpu.registers()[13] == 0x02003000, "Thumb push/pop");
    m->thumb(0x280f); const u32 branch = m->cpu.pc(); m->thumb(0xd001);
    check(m->cpu.pc() == branch + 6, "Thumb conditional branch");
    const u32 call = m->cpu.pc(); m->thumb(0xf000); m->thumb(0xf802);
    check(m->cpu.pc() == call + 8 && m->cpu.registers()[14] == (call + 4 + 1), "Thumb BL pair");
}
void thumbMemory() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0x02004000); m->reg(1, 0xffff80ff); m->reg(2, 0); m->enterThumb();
    m->thumb(0x6001); m->thumb(0x6803); check(m->cpu.registers()[3] == 0xffff80ff, "Thumb word transfer");
    m->thumb(0x5e84); check(m->cpu.registers()[4] == 0xffff80ff, "Thumb signed halfword");
    m->thumb(0x5685); check(m->cpu.registers()[5] == 0xffffffff, "Thumb signed byte");
    const u32 literal = (m->cpu.pc() + 4) & ~3u;
    m->memory.write32(literal + 8, 0x12345678); m->thumb(0x4e02);
    check(m->cpu.registers()[6] == 0x12345678, "Thumb PC relative literal");
}
void unsupportedAndReset() {
    auto m = std::make_unique<Machine>();
    bool rejected = false;
    try { m->arm(0xef000001); } catch (const std::runtime_error& e) {
        rejected = std::string(e.what()).find("2000000") != std::string::npos;
    }
    check(rejected, "SWI reports unsupported with address");
    rejected = false;
    try { m->arm(0xe14f0000); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, "SPSR access must not read CPSR");
    m->cpu.reset(); check(m->cpu.pc() == 0x08000000 && m->cpu.cpsr() == 0x13, "default reset");
    for (unsigned r = 0; r < 15; ++r) check(m->cpu.registers()[r] == 0, "reset clears registers");
}
void statusFlags() {
    auto m = std::make_unique<Machine>();
    m->reg(0, 0xa0000000); m->arm(0xe128f000); // MSR CPSR_f,r0
    check(m->cpu.cpsr() == 0xa0000013, "MSR register flags preserve control");
    m->arm(0xe10f1000); // MRS r1,CPSR
    check(m->cpu.registers()[1] == 0xa0000013, "MRS CPSR");
    m->arm(0xe328f102); // MSR CPSR_f,#0x80000000
    check(m->cpu.cpsr() == 0x80000013, "MSR rotated immediate");
    bool rejected = false;
    try { m->arm(0xe121f000); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected && m->cpu.cpsr() == 0x80000013, "invalid mode writes preserve status");
}
void modeBanking() {
    auto m = std::make_unique<Machine>();
    m->reg(13, 0x03007fe0); m->reg(14, 0x1234); m->reg(8, 88);
    m->reg(0, 0xa00000d2); m->arm(0xe129f000); // Exact failing ROM instruction.
    check(m->cpu.cpsr() == 0xa00000d2 && m->cpu.registers()[13] == 0, "MSR CPSR_fc selects IRQ bank");
    m->reg(13, 0x03007fa0); m->reg(14, 0x5678);
    m->arm(0xe321f013); // MSR CPSR_c,#SVC
    check(m->cpu.registers()[13] == 0x03007fe0 && m->cpu.registers()[14] == 0x1234, "SVC bank restored");
    m->arm(0xe321f012);
    check(m->cpu.registers()[13] == 0x03007fa0 && m->cpu.registers()[14] == 0x5678, "IRQ bank restored");
    m->arm(0xe321f011); check(m->cpu.registers()[8] == 0, "FIQ high bank starts cleared");
    m->reg(8, 99); m->arm(0xe321f017);
    check(m->cpu.registers()[8] == 88, "non-FIQ high bank restored");
    m->reg(13, 17); m->arm(0xe321f01b); m->reg(13, 27);
    m->arm(0xe321f017); check(m->cpu.registers()[13] == 17, "Abort bank restored");
    m->arm(0xe321f01b); check(m->cpu.registers()[13] == 27, "Undefined bank restored");
    m->arm(0xe321f011); check(m->cpu.registers()[8] == 99, "FIQ high bank restored");
    m->arm(0xe321f01f); m->reg(13, 31); m->arm(0xe321f010);
    check(m->cpu.registers()[13] == 31, "User and System share SP");
    m->reg(0, 0x500000d3); m->arm(0xe129f000);
    check(m->cpu.cpsr() == 0x50000010 && m->cpu.registers()[13] == 31, "User can write flags but not control");
    m->cpu.reset(0x02000000); m->arm(0xe321f011);
    check(m->cpu.registers()[8] == 0 && m->cpu.registers()[13] == 0, "reset clears banked registers");
}
}
int main() {
    const struct { const char* name; void (*run)(); } tests[] = {
        {"arithmetic", arithmetic}, {"conditions", conditions}, {"shifts", shifts},
        {"branches", branches}, {"transfers", transfers}, {"multiply", multiply},
        {"block transfers", blockTransfers}, {"Thumb arithmetic/stack", thumbArithmeticAndStack},
        {"Thumb memory", thumbMemory}, {"unsupported/reset", unsupportedAndReset},
        {"status flags", statusFlags}, {"mode banking", modeBanking}
    };
    unsigned failures = 0;
    for (const auto& test : tests) {
        try { test.run(); std::cout << "PASS " << test.name << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << test.name << ": " << e.what() << '\n'; }
    }
    return failures ? 1 : 0;
}
