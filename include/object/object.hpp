#pragma once

#include "system/types.h"

#include "ktl/atomic.hpp"

namespace object
{

using object_counter_type = ktl::atomic<uint64_t>;
static_assert(object_counter_type::is_always_lock_free);

using koid_type = int64_t;
using koid_counter_type = ktl::atomic<koid_type>;
static_assert(koid_counter_type::is_always_lock_free);

template<typename T>
class object
{
 public:
	static object_counter_type kobject_counter_;
	static koid_counter_type koid_counter_;

	object()
		: koid{ koid_counter_.load() }
	{
		++koid_counter_;
		++kobject_counter_;
	}

	virtual ~object()
	{
		--kobject_counter_;
	}

	[[nodiscard]] koid_type get_koid() const
	{
		return koid;
	}

	void set_koid(koid_type _koid)
	{
		koid = _koid;
	}

 private:
	koid_type koid{ 0 };

};

}