#pragma once

#define BIT(n) (1u << (n))

#define DEREF_ADDRESS(ptr) (*(volatile uint32_t*)(ptr))