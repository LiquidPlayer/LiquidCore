//
//  HeapLookup.c
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "HeapObjects.h"

namespace V82JSC_HeapObject {
    
static uint64_t __two[] =
{
    1, // 0000 0000
    4, // 0000 0001
    4, // 0000 0010
    4, // 0000 0011
    1, // 0000 0100
    16, // 0000 0101
    16, // 0000 0110
    16, // 0000 0111
    
    1, // 0000 1000
    16, // 0000 1001
    16, // 0000 1010
    16, // 0000 1011
    1, // 0000 1100
    16, // 0000 1101
    16, // 0000 1110
    16, // 0000 1111
    
    1, // 0001 0000
    4, // 0001 0001
    4, // 0001 0010
    4, // 0001 0011
    1, // 0001 0100
    64, // 0001 0101
    64, // 0001 0110
    64, // 0001 0111
    
    1, // 0001 1000
    64, // 0001 1001
    64, // 0001 1010
    64, // 0001 1011
    1, // 0001 1100
    64, // 0001 1101
    64, // 0001 1110
    64, // 0001 1111

    1, // 0010 0000
    4, // 0010 0001
    4, // 0010 0010
    4, // 0010 0011
    1, // 0010 0100
    64, // 0010 0101
    64, // 0010 0110
    64, // 0010 0111
    
    1, // 0010 1000
    64, // 0010 1001
    64, // 0010 1010
    64, // 0010 1011
    1, // 0010 1100
    64, // 0010 1101
    64, // 0010 1110
    64, // 0010 1111

    1, // 0011 0000
    4, // 0011 0001
    4, // 0011 0010
    4, // 0011 0011
    1, // 0011 0100
    64, // 0011 0101
    64, // 0011 0110
    64, // 0011 0111
    
    1, // 0011 1000
    64, // 0011 1001
    64, // 0011 1010
    64, // 0011 1011
    1, // 0011 1100
    64, // 0011 1101
    64, // 0011 1110
    64, // 0011 1111

    1, // 0100 0000
    4, // 0100 0001
    4, // 0100 0010
    4, // 0100 0011
    1, // 0100 0100
    16, // 0100 0101
    16, // 0100 0110
    16, // 0100 0111
    
    1, // 0100 1000
    16, // 0100 1001
    16, // 0100 1010
    16, // 0100 1011
    1, // 0100 1100
    16, // 0100 1101
    16, // 0100 1110
    16, // 0100 1111

    1, // 0101 0000
    4, // 0101 0001
    4, // 0101 0010
    4, // 0101 0011
    1, // 0101 0100
    0, // 0101 0101
    0, // 0101 0110
    0, // 0101 0111
    
    1, // 0101 1000
    0, // 0101 1001
    0, // 0101 1010
    0, // 0101 1011
    1, // 0101 1100
    0, // 0101 1101
    0, // 0101 1110
    0, // 0101 1111

    1, // 0110 0000
    4, // 0110 0001
    4, // 0110 0010
    4, // 0110 0011
    1, // 0110 0100
    0, // 0110 0101
    0, // 0110 0110
    0, // 0110 0111
    
    1, // 0110 1000
    0, // 0110 1001
    0, // 0110 1010
    0, // 0110 1011
    1, // 0110 1100
    0, // 0110 1101
    0, // 0110 1110
    0, // 0110 1111

    1, // 0111 0000
    4, // 0111 0001
    4, // 0111 0010
    4, // 0111 0011
    1, // 0111 0100
    0, // 0111 0101
    0, // 0111 0110
    0, // 0111 0111
    
    1, // 0111 1000
    0, // 0111 1001
    0, // 0111 1010
    0, // 0111 1011
    1, // 0111 1100
    0, // 0111 1101
    0, // 0111 1110
    0, // 0111 1111

    1, // 1000 0000
    4, // 1000 0001
    4, // 1000 0010
    4, // 1000 0011
    1, // 1000 0100
    16, // 1000 0101
    16, // 1000 0110
    16, // 1000 0111
    
    1, // 1000 1000
    16, // 1000 1001
    16, // 1000 1010
    16, // 1000 1011
    1, // 1000 1100
    16, // 1000 1101
    16, // 1000 1110
    16, // 1000 1111

    1, // 1001 0000
    4, // 1001 0001
    4, // 1001 0010
    4, // 1001 0011
    1, // 1001 0100
    0, // 1001 0101
    0, // 1001 0110
    0, // 1001 0111
    
    1, // 1001 1000
    0, // 1001 1001
    0, // 1001 1010
    0, // 1001 1011
    1, // 1001 1100
    0, // 1001 1101
    0, // 1001 1110
    0, // 1001 1111

    1, // 1010 0000
    4, // 1010 0001
    4, // 1010 0010
    4, // 1010 0011
    1, // 1010 0100
    0, // 1010 0101
    0, // 1010 0110
    0, // 1010 0111
    
    1, // 1010 1000
    0, // 1010 1001
    0, // 1010 1010
    0, // 1010 1011
    1, // 1010 1100
    0, // 1010 1101
    0, // 1010 1110
    0, // 1010 1111

    1, // 1011 0000
    4, // 1011 0001
    4, // 1011 0010
    4, // 1011 0011
    1, // 1011 0100
    0, // 1011 0101
    0, // 1011 0110
    0, // 1011 0111
    
    1, // 1011 1000
    0, // 1011 1001
    0, // 1011 1010
    0, // 1011 1011
    1, // 1011 1100
    0, // 1011 1101
    0, // 1011 1110
    0, // 1011 1111

    1, // 1100 0000
    4, // 1100 0001
    4, // 1100 0010
    4, // 1100 0011
    1, // 1100 0100
    16, // 1100 0101
    16, // 1100 0110
    16, // 1100 0111
    
    1, // 1100 1000
    16, // 1100 1001
    16, // 1100 1010
    16, // 1100 1011
    1, // 1100 1100
    16, // 1100 1101
    16, // 1100 1110
    16, // 1100 1111

    1, // 1101 0000
    4, // 1101 0001
    4, // 1101 0010
    4, // 1101 0011
    1, // 1101 0100
    0, // 1101 0101
    0, // 1101 0110
    0, // 1101 0111
    
    1, // 1101 1000
    0, // 1101 1001
    0, // 1101 1010
    0, // 1101 1011
    1, // 1101 1100
    0, // 1101 1101
    0, // 1101 1110
    0, // 1101 1111

    1, // 1110 0000
    4, // 1110 0001
    4, // 1110 0010
    4, // 1110 0011
    1, // 1110 0100
    0, // 1110 0101
    0, // 1110 0110
    0, // 1110 0111
    
    1, // 1110 1000
    0, // 1110 1001
    0, // 1110 1010
    0, // 1110 1011
    1, // 1110 1100
    0, // 1110 1101
    0, // 1110 1110
    0, // 1110 1111

    1, // 1111 0000
    4, // 1111 0001
    4, // 1111 0010
    4, // 1111 0011
    1, // 1111 0100
    0, // 1111 0101
    0, // 1111 0110
    0, // 1111 0111
    
    1, // 1111 1000
    0, // 1111 1001
    0, // 1111 1010
    0, // 1111 1011
    1, // 1111 1100
    0, // 1111 1101
    0, // 1111 1110
    0, // 1111 1111
};

static uint8_t __trailing_four[] =
{
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    0,
};
static uint8_t __leading_four[] =
{
    0,
    0,
    0,
    0,
    16,
    16,
    16,
    16,
    16,
    0,
};

static uint64_t __eight[] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
};

uint8_t one(uint64_t b)
{
    return ffsll(~b);
}

uint8_t two(uint64_t b)
{
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        __two[bytes[0]]  +
        (__two[bytes[1]] << 8)  +
        (__two[bytes[2]] << 16) +
        (__two[bytes[3]] << 24) +
        (__two[bytes[4]] << 32) +
        (__two[bytes[5]] << 40) +
        (__two[bytes[6]] << 48) +
        (__two[bytes[7]] << 56);
    return ffsll(mask);
}

uint8_t four(uint64_t b)
{
    auto padt = [](uint8_t n) -> uint64_t
    {
        return __trailing_four[__builtin_ctz(0x100 + n)];
    };
    auto padl = [](uint8_t n) -> uint64_t
    {
        int nn = ((((int) n) << 1) + 1) << ((sizeof(int) - sizeof(uint8_t)) * 8 - 1);
        return __leading_four[__builtin_clz(nn)];
    };
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        (padl(bytes[0]) + padt(bytes[0]))  +
        ((padl(bytes[1]) + padt(bytes[1])) << 8 ) +
        ((padl(bytes[2]) + padt(bytes[1])) << 16)  +
        ((padl(bytes[3]) + padt(bytes[3])) << 24) +
        ((padl(bytes[4]) + padt(bytes[4])) << 32) +
        ((padl(bytes[5]) + padt(bytes[5])) << 40) +
        ((padl(bytes[6]) + padt(bytes[6])) << 48) +
        ((padl(bytes[7]) + padt(bytes[7])) << 56);
    return ffsll(mask);
}

uint8_t eight(uint64_t b)
{
    auto pad = [](uint8_t n) -> uint64_t
    {
        return __eight[__builtin_ctz(0x100 + n)];
    };
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        pad(bytes[0]) +
        (pad(bytes[1]) << 8)  +
        (pad(bytes[2]) << 16) +
        (pad(bytes[3]) << 24) +
        (pad(bytes[4]) << 32) +
        (pad(bytes[5]) << 40) +
        (pad(bytes[6]) << 48) +
        (pad(bytes[7]) << 56);
    return ffsll(mask);
}

uint8_t sixteen(uint64_t b)
{
    auto pad = [](uint8_t n) -> uint64_t
    {
        return __eight[__builtin_ctz(0x100 + n)];
    };
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        (pad(bytes[0]) & pad(bytes[1])) +
        ((pad(bytes[2]) & pad(bytes[3])) << 16) +
        ((pad(bytes[4]) & pad(bytes[5])) << 32) +
        ((pad(bytes[6]) & pad(bytes[7])) << 48);
    return ffsll(mask);
}

uint8_t thirtytwo(uint64_t b)
{
    auto pad = [](uint8_t n) -> uint64_t
    {
        return __eight[__builtin_ctz(0x100 + n)];
    };
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        (pad(bytes[0]) & pad(bytes[1]) & pad(bytes[2]) & pad(bytes[3])) +
        ((pad(bytes[4]) & pad(bytes[5]) & pad(bytes[6]) & pad(bytes[7])) << 32);
    return ffsll(mask);
}

uint8_t sixtyfour(uint64_t b)
{
    auto pad = [](uint8_t n) -> uint64_t
    {
        return __eight[__builtin_ctz(0x100 + n)];
    };
    uint8_t *bytes = (uint8_t *) &b;
    uint64_t mask =
        pad(bytes[0]) & pad(bytes[1]) & pad(bytes[2]) & pad(bytes[3]) &
        pad(bytes[4]) & pad(bytes[5]) & pad(bytes[6]) & pad(bytes[7]);
    return ffsll(mask);
}

typedef uint8_t (*Transform)(uint64_t);

static Transform transforms[] = {
    [](uint64_t) -> uint8_t { return 1; },
    one,
    two,
    four,
    eight,
    sixteen,
    thirtytwo,
    sixtyfour
};

Transform transform (uint32_t slots)
{
    return transforms[__builtin_ctz(slots) + 1];
}

}