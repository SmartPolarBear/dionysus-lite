#include "arch/amd64/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"

#include "sys/bootmm.h"
#include "sys/mmu.h"
#include "sys/vm.h"

#include "lib/libc/string.h"

extern "C" char *vectors; // generated by gvectors.py, compiled from apic/vectors.S

static inline void make_gate(uint32_t *idt_head, uint32_t head_offset, void *kva, uint32_t pl, ExceptionType type)
{
    uintptr_t addr = (uintptr_t)kva;
    head_offset *= 4;
    uint32_t trap = static_cast<decltype(trap)>(type); // == ? 0x8F00 : 0x8E00; // TRAP vs INTERRUPT gate;
    idt_head[head_offset + 0] = (addr & 0xFFFF) | ((SEG_KCODE << 3) << 16);
    idt_head[head_offset + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
    idt_head[head_offset + 2] = addr >> 32;
    idt_head[head_offset + 3] = 0;
}

void trap::initialize_trap_vectors(void)
{
    uint32_t *idt = reinterpret_cast<decltype(idt)>(vm::bootmm_alloc());
    memset(idt, 0, vm::BOOTMM_BLOCKSIZE);

    for (size_t i = 0; i < 256; i++)
    {
        make_gate(idt, i, reinterpret_cast<void *>(vectors[i]), 0, IT_INTERRUPT);
    }

    make_gate(idt, 64, reinterpret_cast<void *>(vectors[64]), 3, IT_TRAP);

    lidt(reinterpret_cast<idt_gate *>(idt), vm::BOOTMM_BLOCKSIZE);
}

extern "C" void trap_entry()
{
    return;
}