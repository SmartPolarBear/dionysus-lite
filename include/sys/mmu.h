/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-13 22:54:28
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_MMU_H)
#define __INCLUDE_SYS_MMU_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

constexpr size_t PDENTRIES_COUNT = 512;
constexpr size_t PDX_SHIFT = 22;

constexpr size_t PAGE_SIZE = 4096;

// Page table/directory entry flags
enum PageEntryFlags
{
    PG_P = 0x001,   // Present
    PG_W = 0x002,   // Writeable
    PG_U = 0x004,   // User
    PG_PWT = 0x008, // Write-Through
    PG_PCD = 0x010, // Cache-Disable
    PG_A = 0x020,   // Accessed
    PG_D = 0x040,   // Dirty
    PG_PS = 0x080,  // Page Size
    PG_MBZ = 0x180, // Bits must be zero
};


#endif // __INCLUDE_SYS_MMU_H
