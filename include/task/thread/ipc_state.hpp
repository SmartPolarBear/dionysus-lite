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
// Created by bear on 6/20/21.
//

#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/data/list.hpp"
#include "kbl/data/name.hpp"
#include "kbl/checker/canary.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/concepts.hpp"
#include "kbl/lock/lock_guard.hpp"
#include "kbl/lock/semaphore.hpp"

#include "system/cls.hpp"
#include "system/time.hpp"
#include "system/deadline.hpp"

#include "syscall_handles.hpp"

#include "drivers/apic/traps.h"

#include "task/thread/wait_queue.hpp"
#include "task/thread/cpu_affinity.hpp"
#include "task/thread/user_stack.hpp"

#include "task/scheduler/scheduler_config.hpp"
#include "task/ipc/message.hpp"

#include "memory/address_space.hpp"

#include <compare>

namespace task
{

class ipc_state
{
 public:
	static constexpr size_t MR_SIZE = 64;
	static constexpr size_t BR_SIZE = 33;

	friend class wait_queue;
	friend class thread;

	friend error_code (::sys_ipc_load_message(const syscall::syscall_regs* regs));
	friend error_code (::sys_ipc_store(const syscall::syscall_regs* regs));

	enum [[clang::enum_extensibility(closed)]] states
	{
		IPC_RECEIVING,
		IPC_SENDING,
		IPC_FREE,
	};

	ipc_state() = delete;
	explicit ipc_state(thread* parent) : parent_(parent)
	{
	}
	ipc_state(const ipc_state&) = delete;
	ipc_state& operator=(const ipc_state&) = delete;

	error_code receive_locked(thread* from, const deadline& ddl) TA_REQ(global_thread_lock);

	error_code send_locked(thread* to, const deadline& ddl) TA_REQ(global_thread_lock);

	template<typename T>
	T get_typed_item(size_t index)
	{
		return *reinterpret_cast<T*>(mr_ + index);
	}

	ipc::message_register_type get_mr(size_t index)
	{
		return mr_[index];
	}

	void set_mr(size_t index, ipc::message_register_type value)
	{
		mr_[index] = value;
	}

	ipc::message_register_type get_br(size_t index)
	{
		return br_[index];
	}

	void set_br(size_t index, ipc::message_register_type value)
	{
		br_[index] = value;
	}

	void load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs);

	void store_mrs_locked(size_t st, ktl::span<ipc::message_register_type> mrs);

	void copy_mrs_to_locked(thread* another, size_t st, size_t cnt);

	[[nodiscard]] ipc::message_tag get_message_tag();

	[[nodiscard]] ipc::message_acceptor get_acceptor();

	/// \brief set the message tag to mrs. will reset mr_count_, which influence exist items
	/// \param tag
	void set_message_tag_locked(const ipc::message_tag* tag) noexcept;

	/// \brief set acceptor to brs. will reset mr_count_, which influence exist items
	/// \param acc
	void set_acceptor(const ipc::message_acceptor* acc) noexcept;

 private:
	/// \brief handle extended items like strings and map/grant items
	/// \param to
	/// \return
	error_code send_extended_items(thread* to);

	/// \brief message registers
	ipc::message_register_type mr_[MR_SIZE]{ 0 };

	size_t mr_count_{ 0 };

	/// \brief buffer registers
	ipc::buffer_register_type br_[BR_SIZE]{ 0 };

	size_t br_count_{ 0 };

	thread* parent_{ nullptr };

	states state_{ IPC_FREE };

	kbl::semaphore f_{ 0 };

	kbl::semaphore e_{ 1 };

	wait_queue sender_wait_queue_{};

	wait_queue receiver_wait_queue_{};

	mutable lock::spinlock lock_{ "ipc_state" };
};
}