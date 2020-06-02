#pragma once

#include "arch/amd64/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"

#include "drivers/apic/traps.h"

#include <cstring>
#include <algorithm>

namespace process
{
    enum process_state
    {
        PROC_STATE_UNUSED,
        PROC_STATE_EMBRYO,
        PROC_STATE_SLEEPING,
        PROC_STATE_RUNNABLE,
        PROC_STATE_RUNNING,
        PROC_STATE_ZOMBIE
    };

    using pid = int64_t;

    constexpr size_t PROC_NAME_LEN = 32;

// it should be enough
    constexpr size_t PROC_MAX_COUNT = INT32_MAX;

    constexpr pid PID_MAX = INT64_MAX;

// Per-process state
    struct process_dispatcher
    {
        static constexpr size_t KERNSTACK_PAGES = 2;
        static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PAGE_SIZE;

        char name[PROC_NAME_LEN + 1];

        process_state state;

        pid id;
        pid parent_id;

        size_t runs;
        uintptr_t kstack;

        vmm::mm_struct *mm;

        trap::trap_frame trapframe;
        uintptr_t pgdir_addr;

        size_t flags;

        list_head link;

        process_dispatcher(const char *name, pid id, pid parent_id, size_t flags)
                : state(PROC_STATE_EMBRYO), id(id), parent_id(parent_id),
                  runs(0), kstack(0), mm(nullptr), flags(flags)
        {

            memset(&this->trapframe, 0, sizeof(this->trapframe));

            memmove(this->name, name, std::min((size_t) strlen(name), PROC_NAME_LEN));
        }
    };

    enum process_flags
    {
        PROC_SYS_SERVER,
        PROC_USER,
        PROC_DRIVER,
    };

    enum binary_types
    {
        BINARY_ELF
    };

    void process_init(void);

// create a process
    error_code create_process(IN const char *name,
                              IN size_t flags,
                              IN bool inherit_parent,
                              OUT process_dispatcher **ret);

    error_code process_load_binary(IN process_dispatcher *porc,
                                   IN uint8_t *bin,
                                   IN size_t binsize,
                                   IN binary_types type);

    error_code process_run(IN process_dispatcher *porc);

    // terminate current process
    void process_terminate();

    // terminate the given process
    void process_terminate(IN process_dispatcher *proc);

} // namespace process

extern __thread process::process_dispatcher *current;
