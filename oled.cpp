#include "oled.h"
#include "utils.h"

#include <cstdint> // for types like uintptr_t and uint32_t

void run_oled()
{

    //////////////////////////// clock-gates //////////////////////////////////
    /// the clock enabling must happen before the GPIO registers mods, since
    /// with the clocks off the register writes will be silently dropped.

    static constexpr uintptr_t iRCC = 0x40023800;

    // I2C1 is on APB1 in bit 21
    static constexpr uintptr_t iAPB1ENR = iRCC + 0x40;
    DEREF_ADDRESS(iAPB1ENR) |=  BIT(21);
    // relevant pins are PB6(SCL1) and PB7(SDA1)
    // turn on the GPIOB clock in RCC's AHB1ENR(GPIOBEN)
    static constexpr uintptr_t iAHB1ENR = iRCC + 0x30;
    DEREF_ADDRESS(iAHB1ENR) |=  BIT(1);

    // dummy read-backs used to stall a bit and make sure the peripheral's
    // registers are accessible. Without it the writes can land before the
    // clocks are live.
    (void)DEREF_ADDRESS(iAPB1ENR);
    (void)DEREF_ADDRESS(iAHB1ENR);

    //////////////////////////// GPIO //////////////////////////////////
    static constexpr uintptr_t iGPIOB = 0x40020400;

    static constexpr uintptr_t iGPIOB_MODER = iGPIOB + 0x00;
    // set PB6 and PB7 to alternate func with MODER
    static constexpr uintptr_t iPB6_2bit_MASK = ~(BIT(12) | BIT(13));
    static constexpr uintptr_t iPB7_2bit_MASK = ~(BIT(14) | BIT(15));
    DEREF_ADDRESS(iGPIOB_MODER) = (DEREF_ADDRESS(iGPIOB_MODER)  & iPB6_2bit_MASK) | BIT(13);
    DEREF_ADDRESS(iGPIOB_MODER)  = (DEREF_ADDRESS(iGPIOB_MODER)  & iPB7_2bit_MASK) | BIT(15);

    // set SCL and SDA to be open drain. SCL should be open drain to allow a slaves to pull
    // low when it requires clock stretching without master getting in the way.
    // 0 is push-pull, 1 is open-drain.
    static constexpr uintptr_t iGPIOB_OTYPER = iGPIOB + 0x04;
    DEREF_ADDRESS(iGPIOB_OTYPER) |= BIT(6);
    DEREF_ADDRESS(iGPIOB_OTYPER) |= BIT(7);
    // need to use pull up resistors for open drain. 00-none, 01-pull up, 10-pull down, 11- reserved
    // will use 00 because the slave already has a resistor doing this work
    static constexpr uintptr_t iGPIOB_PUPDR = iGPIOB + 0x0c;
    DEREF_ADDRESS(iGPIOB_PUPDR) &= iPB6_2bit_MASK;
    DEREF_ADDRESS(iGPIOB_PUPDR) &= iPB7_2bit_MASK;
    
    static constexpr uintptr_t iGPIOB_AFRL = iGPIOB + 0x20;
    // according to the datasheet, if we wanna map PB6 and PB7 to I2C1 then the value is 4(AF04).
    // PB6 starts at bit 24(6*4 since each pin has 4 bit of options) and PB7 at bit 28.
    static constexpr uintptr_t iPB6_4bit_MASK = ~(BIT(24) | BIT(25) | BIT(26) | BIT(27));
    static constexpr uintptr_t iPB7_4bit_MASK = ~(BIT(28) | BIT(29) | BIT(30) | BIT(31));
    DEREF_ADDRESS(iGPIOB_AFRL) &= iPB6_4bit_MASK;
    DEREF_ADDRESS(iGPIOB_AFRL) &= iPB7_4bit_MASK;
    DEREF_ADDRESS(iGPIOB_AFRL) |= BIT(26);
    DEREF_ADDRESS(iGPIOB_AFRL) |= BIT(30);
    
}