#include "syscall.h"

#include "sys/mmu.h"
#include "sys/proc.h"
#include "sys/syscall.h"

#include "drivers/debug/kdebug.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "lib/libc/stdio.h"

using namespace syscall;

error_code default_syscall()
{
    return ERROR_NOT_IMPL;
}

error_code sys_hello()
{
    printf("hello sys!\n");
    return ERROR_SUCCESS;
}

extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1] = {
    [0 ... SYSCALL_COUNT] = default_syscall,
    [0] = sys_hello,
};

extern "C" error_code syscall_body()
{
    printf("syscall_body\n");
    return ERROR_SUCCESS;
}

PANIC void syscall::system_call_init()
{
    // check availablity of syscall/sysret
    auto [eax, ebx, ecx, edx] = cpuid(CPUID_INTELFEATURES);

    if (!(edx & (1 << 11)))
    {
        KDEBUG_GENERALPANIC("SYSCALL/SYSRET isn't available.");
    }

    // enanble the syscall/sysret instructions

    auto ia32_EFER_val = rdmsr(MSR_EFER);
    if (!(ia32_EFER_val & 0b1))
    {
        // if SCE bit is not set, set it.
        ia32_EFER_val |= 0b1;
        wrmsr(MSR_EFER, ia32_EFER_val);
    }

    wrmsr(MSR_STAR, (SEGMENTSEL_UNULL << 48ull) | (SEGMENTSEL_KCODE << 32ull));
    wrmsr(MSR_LSTAR, (uintptr_t)syscall_x64_entry);
    wrmsr(MSR_SYSCALL_MASK, EFLAG_TF | EFLAG_DF | EFLAG_IF |
                                EFLAG_IOPL_MASK | EFLAG_AC | EFLAG_NT);
    // we do not support 32bit compatibility mode now
    // TODO:support 32bit compatibility mode
    //// wrmsr(MSR_CSTAR, (uintptr_t)system_call_entry_x86);
}