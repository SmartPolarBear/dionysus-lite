#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "fs/vfs/vfs.hpp"

error_code ext2_block_read(file_system::fs_instance* fs, uint8_t* buf, size_t count);