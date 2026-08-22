#include <cstdint>

volatile int hihi;
struct TestInitArray
{
    TestInitArray()
    {
        hihi = 0xc0ffee;
    }
};

TestInitArray test;

int* iRCC = (int*)0x40023800;
 // must cast to uintptr_t so interger arithmetic is used rather than pointer arithmetic
 // which silently multiplies the added offsets by sizeof(int)
volatile int* iAHB1ENR = (int*)((uintptr_t)iRCC + 0x30);
int* iGPIOC = (int*)0x40020800;
volatile int* iModer = (int*)((uintptr_t)iGPIOC + 0x00);
volatile int* iODR = (int*)((uintptr_t)iGPIOC + 0x14);
int main()
{
    *iAHB1ENR |= 0b100;
    *iModer |= 0x4000000;
    for(unsigned int cnt=0;;++cnt)
    {
        if((cnt/50000) % 2 == 0)
            *iODR |= 0x2000;
        else
            *iODR &= ~0x2000;
    }

    return 0;
}