#ifndef PLAT_H
#define PLAY_H
#include <cstdint>

namespace plat {

/**
 * Atomically sets a bit in specified byte.
 * @param	ptr		Pointer to a byte
 * @param	bit		Bit offset (from 0)
 */
void atomicSetBit(volatile uint8_t *ptr, int bit);

}

#endif
