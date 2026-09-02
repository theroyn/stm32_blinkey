#include "oled.h"
#include "utils.h"

#include <cstdint> // for types like uintptr_t and uint32_t

static constexpr uintptr_t iRCC = 0x40023800;
static constexpr uintptr_t iI2C1 = 0x40005400;
static constexpr uintptr_t iI2C1_CR1 = iI2C1 + 0x00;
static constexpr uintptr_t bitI2C1_CR1_START = BIT(8);
static constexpr uintptr_t bitI2C1_CR1_STOP = BIT(9);
static constexpr uintptr_t iI2C1_DR = iI2C1 + 0x10;
static constexpr uintptr_t iI2C1_SR1 = iI2C1 + 0x14;
static constexpr uintptr_t iI2C1_SR2 = iI2C1 + 0x18;

uint32_t GetClockFreq()
{
    static constexpr uintptr_t iRCC_CFGR = iRCC + 0x08;
    uint32_t iRCC_CFGR_deref = DEREF_ADDRESS(iRCC_CFGR);
    static constexpr uint32_t HSE_VALUE = 25; // WEAct board constant
    uint32_t sysclk = 16;
    int iSWS = (iRCC_CFGR_deref >> 2) & 0b11;
    switch(iSWS)
    {
        case 0: // HSI
        sysclk = 16;
        break;
        case 1: // HSE
        sysclk = HSE_VALUE;
        break;
        case 2: // PLL
        {
            static constexpr uintptr_t iRCC_PLLCFGR = iRCC + 0x04;
            uint32_t iRCC_PLLCFGR_deref = DEREF_ADDRESS(iRCC_PLLCFGR);
            // PLLSRC
            int src = 16;
            if(iRCC_PLLCFGR_deref & BIT(22))
            {
                src = HSE_VALUE;
            }
            uint32_t plln = (iRCC_PLLCFGR_deref >> 6) & 0x1ff; // starting at bit 6 and lasting 9 bits
            uint32_t pllm = iRCC_PLLCFGR_deref & 0b111111; // starting at bit 0 and lasting 6 bits
            uint32_t pllp = (iRCC_PLLCFGR_deref >> 16) & 0b11; // starting at bit 16 and lasting 2 bits
            uint32_t pllpDiv = (pllp + 1) * 2;
            sysclk = (uint32_t)(((uint64_t)src * plln) / (pllm * pllpDiv));
        }
        break;
    }

    uint32_t iHPRE = (iRCC_CFGR_deref >> 4) & 0b1111;
    int nHPREShift = 0; // we will divide by 2^nHPREShift
    switch(iHPRE) // all the 0b0xxx will default to 0
    {
        case 0b1000:
        nHPREShift = 1;
        break;
        case 0b1001:
        nHPREShift = 2;
        break;
        case 0b1010:
        nHPREShift = 3;
        break;
        case 0b1011:
        nHPREShift = 4;
        break;
        case 0b1100:
        nHPREShift = 6;
        break;
        case 0b1101:
        nHPREShift = 7;
        break;
        case 0b1110:
        nHPREShift = 8;
        break;
        case 0b1111:
        nHPREShift = 9;
        break;
    }

    sysclk >>= nHPREShift;
    
    uint32_t iPPRE1 = (iRCC_CFGR_deref >> 10) & 0b111;
    int nPPREShift = 0; // we will divide by 2^nPPREShift
    switch(iPPRE1) // all the 0b0xx will default to 0
    {
        case 0b100:
        nPPREShift = 1;
        break;
        case 0b101:
        nPPREShift = 2;
        break;
        case 0b110:
        nPPREShift = 3;
        break;
        case 0b111:
        nPPREShift = 4;
        break;
    }

    sysclk >>= nPPREShift;

    return sysclk;
}

bool init_clock()
{
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

    return true;
}

bool init_gpio()
{
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

    return true;
}

bool peripheral_conf()
{
    static constexpr uintptr_t iI2C1_CR2 = iI2C1 + 0x04;
    static constexpr uintptr_t iI2C1_CCR = iI2C1 + 0x1c;
    static constexpr uintptr_t iI2C1_TRISE = iI2C1 + 0x20;
    
    static constexpr uintptr_t bitI2C1_CR1_PE = BIT(0);

    // FREQ
    uint32_t iFreqMHz = GetClockFreq();
    DEREF_ADDRESS(iI2C1_CR2) = (DEREF_ADDRESS(iI2C1_CR2) & (~0b111111)) | (iFreqMHz & 0b111111); // FREQ is starting at bit 0 and lasting 6 bits
    // CCR
    DEREF_ADDRESS(iI2C1_CCR) &= ~BIT(15); // SM mode
    uint32_t iFreqKHz = iFreqMHz * 1000;
    uint32_t iTargetFreqKHz = 100; // SM mode is 100 KHz
    uint32_t iCCR = iFreqKHz / (2 * iTargetFreqKHz);
    DEREF_ADDRESS(iI2C1_CCR) = (DEREF_ADDRESS(iI2C1_CCR) & (~0xfff)) | (iCCR & 0xfff); // 12 bits
    // TRISE
    uint32_t iTRISE = iFreqMHz + 1;
    DEREF_ADDRESS(iI2C1_TRISE) = (DEREF_ADDRESS(iI2C1_TRISE) & (~0b111111)) | (iTRISE & 0b111111); // 6 bits
    // should be last in the config as it will cause the others to lock
    DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_PE;

    return true;
}

bool wait_ack()
{
    bool bAck = false;
    bool bNack = false;
    while (!bAck && !bNack)
    {
        if ((DEREF_ADDRESS(iI2C1_SR1) & BIT(1)) != 0)
        {
            // received address matched
            bAck = true;
        }
        else if((DEREF_ADDRESS(iI2C1_SR1) & BIT(10)) != 0)
        {
            // ack failed
            bNack = true;
        }
    }

    return bAck;
}

bool wait_txe()
{
    bool bAck = false;
    bool bNack = false;
    while (!bAck && !bNack)
    {
        if ((DEREF_ADDRESS(iI2C1_SR1) & BIT(7)) != 0)
        {
            // received address matched
            bAck = true;
        }
        else if((DEREF_ADDRESS(iI2C1_SR1) & BIT(10)) != 0)
        {
            // ack failed
            bNack = true;
        }
    }

    return bAck;
}

void handle_ack()
{
        (void)DEREF_ADDRESS(iI2C1_SR1); // needs to read SR1 first
        (void)DEREF_ADDRESS(iI2C1_SR2); // needs to read SR2
}

void handle_nack()
{
    // stop
    DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_STOP;
    // clear AF bit
    DEREF_ADDRESS(iI2C1_SR1) &= ~BIT(10);
}

bool send(uint8_t iControl, uint8_t* pData, uint32_t nData, uint32_t nRepeat = 1)
{
    // wait for SR2 BUSY bit to clear from last STOP
    while((DEREF_ADDRESS(iI2C1_SR2) & BIT(1)) != 0) { }

    // start
    DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_START;

    // poll until interface became master(SB flipped to 1)
    while((DEREF_ADDRESS(iI2C1_SR1) & BIT(0)) == 0) { }

    // send address
    DEREF_ADDRESS(iI2C1_DR) = (0x3c << 1) | 0; // 0x3c SSD1306 oled's address.

    bool bAck = wait_ack();

    if(!bAck)
    {
        handle_nack();
        return false;
    }

    handle_ack();

    DEREF_ADDRESS(iI2C1_DR) = iControl;

    bAck = wait_txe();

    if(!bAck)
    {
        handle_nack();
        return false;
    }

    for(uint32_t iRepeat = 0; iRepeat < nRepeat; ++iRepeat)
    {
        for(uint32_t iDatum = 0; iDatum < nData; ++iDatum)
        {
            DEREF_ADDRESS(iI2C1_DR) = pData[iDatum];

            bAck = wait_txe();

            if(!bAck)
            {
                handle_nack();
                return false;
            }
        }
    }

    // poll for BTF
    while((DEREF_ADDRESS(iI2C1_SR1) & BIT(2)) == 0) { }

    // stop
    DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_STOP;

    return true;
}

void run_oled()
{
    //////////////////////////// clock-gates //////////////////////////////////
    /// the clock enabling must happen before the GPIO registers mods, since
    /// with the clocks off the register writes will be silently dropped.
    if(!init_clock())
        return;

    //////////////////////////// GPIO //////////////////////////////////
    if(!init_gpio())
        return;
    
    //////////////////////////// peripheral config //////////////////////////////////
    if(!peripheral_conf())
        return;
    
    //////////////////////////// first ACK //////////////////////////////////
    
    // start
    DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_START;
    // poll until interface became master(SB flipped to 1)
    while((DEREF_ADDRESS(iI2C1_SR1) & BIT(0)) == 0) { }

    // send address
    DEREF_ADDRESS(iI2C1_DR) = (0x3c << 1) | 0; // 0x3c SSD1306 oled's address.

    bool bAck = wait_ack();

    if(bAck)
    {
        handle_ack();
        // stop
        DEREF_ADDRESS(iI2C1_CR1) |= bitI2C1_CR1_STOP;
    }
    else
    {
        handle_nack();

        return;
    }

    /*
    0xAE        display OFF
    0xD5 0x80   clock divide / osc freq
    0xA8 0x3F   multiplex ratio = 63 (64 rows)
    0xD3 0x00   display offset = 0
    0x40        start line = 0
    0x8D 0x14   charge pump ON      ← CRITICAL
    0x20 0x00   addressing mode = horizontal
    0xA1        segment remap
    0xC8        COM scan direction remapped

    0xDA 0x12   COM pins config
    0x81 0x7F   contrast
    0xD9 0xF1   pre-charge
    0xDB 0x40   VCOMH deselect

    0xA4        output follows RAM
    0xA6        normal (non-inverted)
    0xAF        display ON
    */
    uint8_t arrCommands[256] = {0};
    arrCommands[0] = 0xae; // iDisplayOff
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0xd5; arrCommands[1] = 0x80; // clock divide / osc freq
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xA8; arrCommands[1] = 0x3F; // multiplex ratio = 63 (64 rows)
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xD3; arrCommands[1] = 0x00; // display offset = 0
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0x40; // start line = 0
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0x8D; arrCommands[1] = 0x14; // charge pump ON
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0x20; arrCommands[1] = 0x00; // addressing mode = horizontal
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xA1; // segment remap
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0xC8; // COM scan direction remapped
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0xDA; arrCommands[1] = 0x12; // COM pins config
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0x81; arrCommands[1] = 0x7F; // contrast
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xD9; arrCommands[1] = 0xF1; // pre-charge
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xDB; arrCommands[1] = 0x40; // VCOMH deselect
    if(!send(0x00, arrCommands, 2)) return;
    arrCommands[0] = 0xA4; // output follows RAM
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0xA6; // normal (non-inverted)
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0xAF; // display ON
    if(!send(0x00, arrCommands, 1)) return;
    arrCommands[0] = 0x00; // all pixels off
    if(!send(0x40, arrCommands, 1, 1024)) return;
    arrCommands[0] = 0xff; // all pixels on
    if(!send(0x40, arrCommands, 1, 1024)) return;
    
}