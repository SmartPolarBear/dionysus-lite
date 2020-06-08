#include "syscall.h"

#include "system/mmu.h"
#include "system/proc.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

using namespace syscall;

size_t get_nth_arg(const syscall_regs *regs, size_t n)
{
    switch (n)
    {
        case 0:
            return regs->rdi;
        case 1:
            return regs->rsi;
        case 2:
            return regs->rdx;
        case 3:
            return regs->r10;
        case 4:
            return regs->r8;
        case 5:
            return regs->r9;
        default:
            KDEBUG_RICHPANIC("System call can have not more than 4 args.", "Syscall", false, "");
    }
}

size_t get_syscall_number(const syscall_regs *regs)
{
    return regs->rax;
}




extern "C" error_code syscall_body()
{
    // saved registers is right in the stack
    uintptr_t sp = 0;
    asm volatile ("mov %%rsp,%0":"=r"(sp));

    // FIXME: r11 value appear under r14. needs further investigation.
    const syscall_regs *regs = reinterpret_cast<syscall_regs *>(sp + sizeof(syscall_regs) + sizeof(uint64_t));

    KDEBUG_ASSERT(regs != nullptr);

    size_t syscall_no = get_syscall_number(regs);  // first parameter

    if (syscall_no > SYSCALL_COUNT)
    {
        KDEBUG_FOLLOWPANIC("Syscall number out of range.");
    }

    auto ret = syscall_table[syscall_no](regs);

    return ret;
}