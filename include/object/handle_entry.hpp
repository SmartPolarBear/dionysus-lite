#pragma once

#include "system/types.h"

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"
#include "kbl/data/name.hpp"

#include "object/ref_counted.hpp"
#include "object/dispatcher.hpp"
#include "object/public/handle_type.hpp"

#include "ktl/shared_ptr.hpp"

#include <any>
#include <utility>

namespace object
{

enum class [[clang::enum_extensibility(closed)]] handle_table_type
{
	LOCAL, GLOBAL
};

class handle_table;
class handle_entry;

struct handle_entry_deleter
{
	void operator()(handle_entry* e);
};

using handle_entry_owner = ktl::unique_ptr<handle_entry, handle_entry_deleter>;

class handle_entry final
{
 public:
	static constexpr size_t NAME_LEN = 16;

	friend class handle_table;
	friend class object_manager;
	friend struct handle_entry_deleter;

	handle_entry() = delete;

	~handle_entry()
	{
		canary_.assert();
		if (ptr_->release())
		{
			delete ptr_;
		}
	}

	[[nodiscard]] static handle_entry_owner create(ktl::string_view name, const dispatcher* obj);

	[[nodiscard]] static handle_entry_owner duplicate(handle_entry* h);

	static void release(handle_entry_owner h);

	[[nodiscard]] dispatcher* object() const
	{
		return ptr_;
	}


	bool operator==(const handle_entry& another) const
	{
		return ptr_ == another.ptr_;
	}

	bool operator!=(const handle_entry& another) const
	{
		return !operator==(another);
	}
 private:
	static void release(handle_entry* h);

	handle_entry(const handle_entry& another)
		: ptr_(another.ptr_)
	{
		name_.set(another.name_.data());

		ptr_->add_ref();
	}

	handle_entry(handle_entry&& another) noexcept
		: ptr_(std::exchange(another.ptr_, nullptr))
	{
		name_.set(another.name_.data());
		another.name_.set("obj");
	}

	explicit handle_entry(const dispatcher* obj, ktl::string_view name = "obj")
		: ptr_(const_cast<dispatcher*>(obj))
	{
		name_.set(name);
	}

	kbl::canary<kbl::magic("HENT")> canary_{};

	dispatcher* ptr_{ nullptr };

	handle_table* parent_{ nullptr };

	kbl::name<NAME_LEN> name_{};

	[[maybe_unused]] rights_type rights_{ 0 };

	koid_type owner_process_id{ -1 };

	handle_type value_{ INVALID_HANDLE_VALUE };
};

}