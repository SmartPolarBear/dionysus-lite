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
static error_code default_trap_handle([[maybe_unused]] trap_info info)
{
    //TODO: the handle doesn't exist
    return ERROR_SUCCESS;
}

static error_code spurious_trap_handle([[maybe_unused]] trap_info info)
{
    return ERROR_SUCCESS;
}

extern "C" void *vectors[]; // generated by gvectors.py, compiled from apic/vectors.S

// defined in trapentry_asm.S
extern "C" void trap_entry();
extern "C" void trap_ret();

// defined below. used by trap_entry to futuer process a trap
extern "C" void trap_body(trap_info info);

static inline void make_gate(uint32_t *idt_head, uint32_t head_offset, void *kva, uint32_t pl, ExceptionType type)
{
    uintptr_t addr = (uintptr_t)kva;
    head_offset *= 4;
    uint32_t trap = static_cast<decltype(trap)>(type);
    idt_head[head_offset + 0] = (addr & 0xFFFF) | ((SEG_KCODE << 3) << 16);
    idt_head[head_offset + 1] = (addr & 0xFFFF0000) | trap | ((pl & 3) << 13); // P=1 DPL=pl
    idt_head[head_offset + 2] = addr >> 32;
    idt_head[head_offset + 3] = 0;
}

void trap::init_trap(void)
{
    uint32_t *idt = reinterpret_cast<uint32_t *>(new BLOCK<PMM_PAGE_SIZE>);
    memset(idt, 0, PMM_PAGE_SIZE);

    for (size_t i = 0; i < 256; i++)
    {
        make_gate(idt, i, vectors[i], DPL_KERNEL, IT_INTERRUPT);
    }

    make_gate(idt, 64, vectors[64], DPL_USER, IT_TRAP);

    lidt(reinterpret_cast<idt_gate *>(idt), PMM_PAGE_SIZE);

    spinlock_initlock(&handle_table.lock, "traphandles");

    spinlock_acquire(&handle_table.lock);

    for (auto &handle : handle_table.trap_handles)
    {
        if (handle.handle == nullptr)
        {
            handle.handle = default_trap_handle;
        }
    }

    handle_table.trap_handles[trap::TRAP_IRQ0 + trap::IRQ_SPURIOUS].handle = spurious_trap_handle;

    spinlock_release(&handle_table.lock);
}

// returns the old handle
trap_handle trap::trap_handle_regsiter(size_t number, trap_handle handle)
{
    spinlock_acquire(&handle_table.lock);

    trap_handle old = trap_handle{handle_table.trap_handles[number]};
    handle_table.trap_handles[number] = handle;

    spinlock_release(&handle_table.lock);

    return old;
}

extern "C" void trap_body(trap_info info)
{
    // check if the trap number is out of range
    if (info.trap_num >= TRAP_NUMBERMAX || info.trap_num < 0)
    {
        KDEBUG_RICHPANIC("trap number is out of range",
                         "KERNEL PANIC: TRAP",
                         false, "The given trap number is %d", info.trap_num);
    }

    // it should be assigned with the defualt handle when initialized
    KDEBUG_ASSERT(handle_table.trap_handles[info.trap_num].handle != nullptr);

    // call the handle
    auto error = handle_table.trap_handles[info.trap_num].handle(info);

    // finish the trap handle
    local_apic::write_eoi();

    if (error != ERROR_SUCCESS)
    {
        console::printf("** trap number:%d, error code:%d.\n", info.trap_num, error);
        KDEBUG_FOLLOWPANIC("trap handle reports an error.");
    }
}
