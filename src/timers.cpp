#include "gba/timers.hpp"

namespace gba {

void Timers::step(int cycles) {
    accumulated_cycles_ += cycles;

    // TODO:
    // Implement TM0-TM3 counters, prescalers,
    // cascading, overflow, and interrupts.
}

} // namespace gba
