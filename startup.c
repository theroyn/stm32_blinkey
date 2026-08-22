#include <stdint.h>


// externs from linker
extern int _estack; // initial stack pointer, located at the end of the RAM
extern int _sData; // start of .data on RAM
extern int _eData; // end of .data on RAM
extern int _siData; // start of .data on FLASH
extern int _sbData; // start of .bss on RAM
extern int _ebData; // end of .bss on RAM
// forward declarations
void reset_handler();
void DefaultHandler();
int main();
void __libc_init_array(void);
// weak fucks
__attribute__((weak, alias("DefaultHandler"))) void NMI_handler();
__attribute__((weak, alias("DefaultHandler"))) void HardFault_handler();
__attribute__((weak, alias("DefaultHandler"))) void MemManage_handler();
__attribute__((weak, alias("DefaultHandler"))) void BusFault_handler();
__attribute__((weak, alias("DefaultHandler"))) void UsageFault_handler();
__attribute__((weak, alias("DefaultHandler"))) void SVCall_handler();
__attribute__((weak, alias("DefaultHandler"))) void DebugMonitor_handler();
__attribute__((weak, alias("DefaultHandler"))) void PendSV_handler();
__attribute__((weak, alias("DefaultHandler"))) void Systick_Handler();

__attribute__ ((section(".isr_vector"), used))
const uintptr_t  g_ISRVec[]=
{
    (uintptr_t)&_estack, // 0x0000
    (uintptr_t)&reset_handler, // 0x0004
    (uintptr_t)&NMI_handler, // 0x0008
    (uintptr_t)&HardFault_handler, // 0x000c
    (uintptr_t)&MemManage_handler, // 0x0010
    (uintptr_t)&BusFault_handler, // 0x0014
    (uintptr_t)&UsageFault_handler, // 0x0018
    0, 0, 0, 0, // reserved 0x001C-0x002b
    (uintptr_t)&SVCall_handler, // 0x002c
    (uintptr_t)&DebugMonitor_handler, // 0x0030
    0, // reserved 0x0034
    (uintptr_t)&PendSV_handler, // 0x0038
    (uintptr_t)&Systick_Handler, // 0x003c
    // ...
    // last element at 0x0194
};

void DefaultHandler()
{
    for (;;);
}

void reset_handler()
{
    {
        int* pDstData=&_sData;
        int* pSrcData=&_siData;
        while(pDstData < &_eData)
        {
            *(pDstData++)= *(pSrcData++);
        }
    }
    {
        int* pDstData=&_sbData;
        while(pDstData < &_ebData)
        {
            *(pDstData++)= 0;
        }
    }

    __libc_init_array();

    main();

    for(;;);
}