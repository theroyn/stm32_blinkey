#include "blinky.h"

#include <cstdint> // for types like uintptr_t and uint32_t

constexpr uintptr_t iRCC = 0x40023800;
constexpr uintptr_t iAHB1ENR = iRCC + 0x30;
constexpr uintptr_t iGPIOC = 0x40020800;
constexpr uintptr_t iModer = iGPIOC + 0x00;
constexpr uintptr_t iODR = iGPIOC + 0x14;

void run_blinky()
{
    *(volatile uint32_t*)iAHB1ENR |= 0b100;
    *(volatile uint32_t*)iModer = (*(volatile uint32_t*)iModer & ~0xc000000) | 0x4000000; // zero-out the 2-bit mask and set bit 26
    for(unsigned int cnt=0;;++cnt)
    {
        if((cnt/500000) % 2 == 0)
            *(volatile uint32_t*)iODR |= 0x2000;
        else
            *(volatile uint32_t*)iODR &= ~0x2000;
    }
}