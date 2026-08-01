#pragma once

#include <cstdint>

struct PPCContext;

namespace band3 {

struct Symbol {
    uint32_t guest_addr_ = 0;

    Symbol() = default;
    Symbol(PPCContext& ctx, uint8_t* base, const char* name);

    uint32_t value(uint8_t* base) const;

    uint32_t guest_addr() const { return guest_addr_; }
};

}
