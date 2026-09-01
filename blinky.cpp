#include "blinky.h"
#include "utils.h"

#include <cstdint> // for types like uintptr_t and uint32_t

constexpr uintptr_t iRCC = 0x40023800;
constexpr uintptr_t iAHB1ENR = iRCC + 0x30;
constexpr uintptr_t iGPIOC = 0x40020800;
constexpr uintptr_t iModer = iGPIOC + 0x00;
constexpr uintptr_t iODR = iGPIOC + 0x14;

void run_blinky()
{
    // the blue LED's pin is PC13.
    // enable clock for GPIOC, which is on AHB bit 2(0 index) so its 0b0100.
    // iAHB1ENR: Advanced High-Performance Bus 1 Peripheral Clock Enable Register
    *(volatile uint32_t*)iAHB1ENR |= BIT(2);
    
    // PC13 is in GPIOC. iModer has 2 bits per pin, so pin 13 is at bits 26,27.
    // the mapping is 00-input, 01-output, 10-alternate function, 11-analog.
    // in hex its 0x4000000 and 0x8000000. their mask(both set '11') is 0xc000000.
    // we want output so we set the 1st bit.
    uint32_t iMask = ~(BIT(26) | BIT(27));
    *(volatile uint32_t*)iModer = (*(volatile uint32_t*)iModer & iMask) | BIT(26); // zero-out the 2-bit mask and set bit 26
    // *(volatile uint32_t*)iModer = (*(volatile uint32_t*)iModer & ~0xc000000) | 0x4000000; // zero-out the 2-bit mask and set bit 26
    for(unsigned int cnt=0;;++cnt)
    {
        if((cnt/50000) % 2 == 0)
            *(volatile uint32_t*)iODR |= BIT(13); // set bit 13
        else
            *(volatile uint32_t*)iODR &= ~BIT(13); // unset bit 13
    }
}