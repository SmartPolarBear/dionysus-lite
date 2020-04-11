#pragma once

#include "sys/types.h"

static inline uintptr_t rcr2(void)
{
    uintptr_t val;
    asm volatile("mov %%cr2,%0"
                 : "=r"(val));
    return val;
}

static inline uintptr_t rcr3(void) {
    uintptr_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r" (cr3) :: "memory");
    return cr3;
}

static inline void lcr3(uintptr_t val)
{
    asm volatile("mov %0,%%cr3"
                 :
                 : "r"(val));
}

//read eflags
static inline size_t read_eflags(void)
{
    size_t eflags = 0;
    asm volatile("pushf; pop %0"
                 : "=r"(eflags));
    return eflags;
}

