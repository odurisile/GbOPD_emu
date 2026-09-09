#pragma once

namespace gba {

class Timers {
public:
    void step(int cycles);

private:
    int accumulated_cycles_ = 0;
};

} // namespace gba
