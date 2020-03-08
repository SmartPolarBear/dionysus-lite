#include "arch/amd64/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "sys/error.h"
#include "sys/mmu.h"
#include "sys/pmm.h"
#include "sys/vmm.h"

#include "lib/libc/string.h"

using trap::trap_handle;
using trap::TRAP_NUMBERMAX;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initlock;
using lock::spinlock_release;

struct
{
    spinlock lock;
    trap_handle trap_handles[TRAP_NUMBERMAX] = {};
} handle_table;

// default handle of trap
static error_code default_trap_handle([[maybe_unused]] trap_frame info)
{
    // TODO: the handle doesn't exist

    return ERROR_SUCCESS;
}

static error_code spurious_trap_handle([[maybe_unused]] trap_frame info)
{
    return ERROR_SUCCESS;
}

extern "C" void *vectors[]; // generated by gvectors.py, compiled from apic/vectors.S

// defined below. used by trap_entry to futuer process a trap
extern "C" void trap_body(trap_frame info);

static inline void make_gate(uint32_t *idt_head, uint32_t head_offset, void *kva, uint32_t pl, exception_type type)
{
    uintptr_t addr = (uintptr_t)kva;
    head_offset *= 4;
    uint32_t trap = static_cast<decltype(trap)>(type);
    idt_head[head_offset + 0] = (addr & 0xFFFF) | ((SEG_KCODE << 3) << 16);
    idt_head[head_offset + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
    idt_head[head_offset + 2] = addr >> 32;
    idt_head[head_offset + 3] = 0;
}

PANIC void trap::init_trap(void)
{
    uint32_t *idt = reinterpret_cast<uint32_t *>(new BLOCK<PMM_PAGE_SIZE>);
    memset(idt, 0, PMM_PAGE_SIZE);

    for (size_t i = 0; i < 256; i++)
    {
        make_gate(idt, i, vectors[i], DPL_KERNEL, IT_INTERRUPT);
    }

    make_gate(idt, 64, vectors[64], DPL_USER, IT_TRAP);

    lidt((uintptr_t)idt, PMM_PAGE_SIZE);

    spinlock_initlock(&handle_table.lock, "traphandles");

    spinlock_acquire(&handle_table.lock);

    for (auto &handle : handle_table.trap_handles)
    {
        if (handle.handle == nullptr)
        {
            handle.handle = default_trap_handle;
        }
    }

    handle_table.trap_handles[trap::irq_to_trap_number(IRQ_SPURIOUS)].handle = spurious_trap_handle;

    spinlock_release(&handle_table.lock);
}

// returns the old handle
PANIC trap_handle trap::trap_handle_regsiter(size_t number, trap_handle handle)
{
    spinlock_acquire(&handle_table.lock);

    trap_handle old = trap_handle{handle_table.trap_handles[number]};
    handle_table.trap_handles[number] = handle;

    spinlock_release(&handle_table.lock);

    return old;
}

extern "C" void trap_body(trap_frame info)
{
    // check if the trap number is out of range
    if (info.trap_num >= TRAP_NUMBERMAX || info.trap_num < 0)
    {
        KDEBUG_RICHPANIC("trap number is out of range", "KERNEL PANIC: TRAP", false, "The given trap number is %d",
                         info.trap_num);
    }

    // it should be assigned with the defualt handle when initialized
    KDEBUG_ASSERT(handle_table.trap_handles[info.trap_num].handle != nullptr);

    // call the handle
    auto error = handle_table.trap_handles[info.trap_num].handle(info);

    // finish the trap handle
    local_apic::write_eoi();

    if (error != ERROR_SUCCESS)
    {
        KDEBUG_RICHPANIC_CODE(error, true, "");
    }
}

// only works after gdt installation
void trap::pushcli(void)
{
    auto eflags = read_eflags();

    cli();
    if (cpu->nest_pushcli_depth++ == 0)
    {
        cpu->intr_enable = eflags & EFLAG_IF;
    }
}

// only works after gdt installation

void trap::popcli(void)
{
    if (read_eflags() & EFLAG_IF)
    {
        KDEBUG_RICHPANIC("Can't be called if interrupts are enabled",
                         "KERNEL PANIC: SPINLOCK",
                         false,
                         "");
    }

    --cpu->nest_pushcli_depth;
    KDEBUG_ASSERT(cpu->nest_pushcli_depth >= 0);

    if (cpu->nest_pushcli_depth == 0 && cpu->intr_enable)
    {
        sti();
    }
}