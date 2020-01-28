/*
 * Last Modified: Tue Jan 28 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#include "sys/allocators/buddy_alloc.h"
#include "sys/allocators/slab_alloc.h"

#include "drivers/debug/kdebug.h"

#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"

using allocators::slab_allocator::CACHE_NAME_MAXLEN;

using allocators::slab_allocator::slab;
using allocators::slab_allocator::slab_bufctl;
using allocators::slab_allocator::slab_cache;

// buddy
using allocators::buddy_allocator::buddy_alloc;
using allocators::buddy_allocator::buddy_free;

using libk::list_add;
using libk::list_empty;
using libk::list_for_each;
using libk::list_init;
using libk::list_remove;

constexpr size_t SIZED_CACHE_COUNT = 16;
constexpr size_t BLOCK_SIZE = 4096;

list_head cache_head;

slab_cache cache_cache;
slab_cache *sized_caches[SIZED_CACHE_COUNT];
size_t sized_cache_count = 0;

static inline constexpr size_t cache_obj_count(size_t obj_size)
{
    return BLOCK_SIZE / (sizeof(slab_bufctl) + obj_size);
}

static inline void *slab_cache_grow(slab_cache *cache)
{
    auto block = buddy_alloc(BLOCK_SIZE);
    if (block == nullptr)
    {
        return block;
    }

    slab *slb = reinterpret_cast<decltype(slb)>(block);
    slb->cache = cache;
    slb->next_free = 0;
    slb->inuse = 0;

    list_add(&slb->slab_link, &cache->free);

    slb->obj_ptr = reinterpret_cast<decltype(slb->obj_ptr)>(((char *)block) + sizeof(slab));

    void *obj = slb->obj_ptr;
    for (size_t i = 0; i < cache->obj_count; i++)
    {
        if (cache->ctor)
        {
            cache->ctor(obj, cache, cache->obj_size);
        }

        obj = (void *)((char *)obj + cache->obj_size);
        slb->freelist[i] = i + 1;
    }
    slb->freelist[cache->obj_count - 1] = -1;

    return slb;
}

static inline void slab_destory(slab_cache *cache, slab *slb)
{
    void *obj = slb->obj_ptr;
    for (size_t i = 0; i < cache->obj_count; i++)
    {
        if (cache->dtor)
        {
            cache->dtor(obj, cache, cache->obj_size);
        }
    }

    list_remove(&slb->slab_link);
    buddy_free(slb);
}

void allocators::slab_allocator::slab_init(void)
{
    cache_cache.obj_size = sizeof(decltype(cache_cache));
    cache_cache.obj_count = cache_obj_count(cache_cache.obj_size);
    cache_cache.ctor = nullptr;
    cache_cache.dtor = nullptr;

    auto cache_cache_name = "cache_cache";
    strncpy(cache_cache.name, cache_cache_name, CACHE_NAME_MAXLEN);

    list_init(&cache_cache.full);
    list_init(&cache_cache.partial);
    list_init(&cache_cache.free);

    list_init(&cache_head);
    list_add(&cache_cache.cache_link, &cache_head);

    char sized_cache_name[CACHE_NAME_MAXLEN];
    for (size_t sz = 16; sz < BLOCK_SIZE; sz *= 2)
    {
        memset(sized_cache_name, 0, sizeof(sized_cache_name));
        sized_cache_name[0] = 's';
        sized_cache_name[1] = 'i';
        sized_cache_name[2] = 'z';
        sized_cache_name[4] = '-';
        sized_cache_name[5] = '-';

        [[maybe_unused]] size_t len = itoa_ex(sized_cache_name + 4, sz, 10);
        sized_caches[sized_cache_count++] = slab_cache_create(sized_cache_name, sz, nullptr, nullptr);
    }
}

slab_cache *allocators::slab_allocator::slab_cache_create(const char *name, size_t size, ctor_type ctor, dtor_type dtor)
{
    KDEBUG_ASSERT(size < BLOCK_SIZE - sizeof(slab_bufctl));
    slab_cache *ret = reinterpret_cast<decltype(ret)>(slab_cache_alloc(&cache_cache));

    if (ret != nullptr)
    {
        ret->obj_size = size;
        ret->obj_count = cache_obj_count(size);

        ret->ctor = ctor;
        ret->dtor = dtor;

        strncpy(ret->name, name, CACHE_NAME_MAXLEN);

        list_init(&ret->full);
        list_init(&ret->partial);
        list_init(&ret->free);

        list_add(&ret->cache_link, &cache_head);
    }

    return ret;
}

void *allocators::slab_allocator::slab_cache_alloc(slab_cache *cache)
{
    list_head *entry = nullptr;
    if (!list_empty(&cache->partial))
    {
        entry = cache->partial.next;
    }
    else
    {
        if (list_empty(&cache->free) && slab_cache_grow(cache) == nullptr)
        {
            return nullptr;
        }

        entry = cache->free.next;
    }

    list_remove(entry);
    slab *slb = list_entry(entry, slab, slab_link);

    void *ret = (void *)(((uint8_t *)slb->obj_ptr) + slb->next_free * cache->obj_size);

    slb->inuse++;
    slb->next_free = slb->freelist[slb->next_free];

    if (slb->inuse == cache->obj_count)
    {
        list_add(&cache->full, entry);
    }
    else
    {
        list_add(&cache->partial, entry);
    }

    return ret;
}
