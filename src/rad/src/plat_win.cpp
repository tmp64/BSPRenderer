#include <Windows.h>
#include "plat.h"

void plat::atomicSetBit(volatile uint8_t *ptr, int bit) {
	constexpr size_t LONG_SIZE = sizeof(LONG);
    uintptr_t intptr = reinterpret_cast<uintptr_t>(ptr);

    bit = bit + (intptr % LONG_SIZE) * 8;
	intptr = (intptr / LONG_SIZE) * LONG_SIZE;
	volatile LONG *ptr2 = reinterpret_cast<volatile LONG *>(intptr);

	_interlockedbittestandset(ptr2, bit);
}
