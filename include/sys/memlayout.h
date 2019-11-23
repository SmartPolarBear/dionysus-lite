/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:14
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-23 13:39:31
 * @ Description:
 */
#if !defined(__INCLUDE_SYS_MEMLAYOUT_H)
#define __INCLUDE_SYS_MEMLAYOUT_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

// user's address space limit
constexpr size_t USER_ADDRESS_SPACE_LIMIT = 0x00007fffffffffff;

// kernel's address place begin
constexpr size_t KERNEL_ADDRESS_SPACE_BASE = 0xFFFF800000000000;

// the max value for a valid address
constexpr uintptr_t VIRTUALADDR_LIMIT = 0xFFFFFFFFFFFFFFFF;

// remap of physical memory
constexpr uintptr_t PHYREMAP_VIRTUALBASE = 0xffff888000000000;
constexpr uintptr_t PHYREMAP_VIRTUALEND = 0xffffc87fffffffff;
constexpr size_t PHYMEMORY_SIZE = PHYREMAP_VIRTUALEND - PHYREMAP_VIRTUALBASE + 1;

// map kernel, from physical address 0 to 2GiB
constexpr uintptr_t KERNEL_VIRTUALBASE = 0xFFFFFFFF80000000;
constexpr uintptr_t KERNEL_VIRTUALEND = VIRTUALADDR_LIMIT;
constexpr uintptr_t KERNEL_SIZE = KERNEL_VIRTUALEND - KERNEL_VIRTUALBASE + 1;
// Note: the multiboot info will be placed just after kernel
// Be greatly cautious not to overwrite it !!!!
constexpr uintptr_t KERNEL_VIRTUALLINK = 0xFFFFFFFF80100000;

// for memory-mapped IO
constexpr uintptr_t DEVICE_VIRTUALBASE = 0xFFFFFFFF40000000;
constexpr uintptr_t DEVICE_PHYSICALBASE = 0xFE000000;

// convert with uintptr_t, defined in vm_utils.cc
extern uintptr_t V2P(uintptr_t x);
extern uintptr_t P2V(uintptr_t x);
uintptr_t P2V_KERNEL(uintptr_t x);
uintptr_t P2V_PHYREMAP(uintptr_t x);

template <typename P>
static inline P V2P_WO(P a)
{
    return (P)(V2P((uintptr_t)a));
}

template <typename P>
static inline P P2V_WO(P a)
{
    return (P)(P2V((uintptr_t)a));
}

template <typename P>
static inline P V2P(void *a)
{
    return (P)(V2P((uintptr_t)a));
}

template <typename P>
static inline P P2V(void *a)
{
    return (P)(P2V((uintptr_t)a));
}

#endif // __INCLUDE_SYS_MEMLAYOUT_H
