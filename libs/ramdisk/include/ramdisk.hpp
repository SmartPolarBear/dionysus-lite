// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by bear on 6/29/21.
//

#pragma once

#ifndef __cplusplus
#error "This file is only for C++"
#endif

#include <cstdint>

enum ramdisk_flags : uint64_t
{
	FLAG_ARCH_AMD64 = 0b1,
};

enum ramdisk_item_flags : uint64_t
{
	FLAG_AP_BOOT = 0b1,
};

static inline constexpr uint64_t RAMDISK_HEADER_MAGIC = 0x20011204;

struct ramdisk_item
{
	char name[16];
	uint64_t flags;
	uint64_t offset;
	uint64_t size;
}__attribute__((packed));

static_assert(sizeof(ramdisk_item) == 40);


struct ramdisk_header
{
	char name[16];
	uint64_t magic;
	uint64_t flags;
	uint64_t checksum;
	uint64_t size;
	uint64_t count;
	ramdisk_item items[0];
}__attribute__((packed));

static_assert(sizeof(ramdisk_header) == 56);
